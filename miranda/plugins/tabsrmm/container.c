/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

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

message window container implementation
Written by Nightwish (bof@hell.at.eu.org), Jul 2004

License: GPL

Just a few notes, how the container "works".
--------------------------------------------

The container has only one child window, a tab control with the id IDC_MSGTABS.
Each tab is assigned to a message window - the message windows are shown or hidden,
depending on the current tab selection. They are never disabled, just sent to the
background if not active.

Each tab "knows" the hwnd handle of his message dialog child, because at creation,
this handle is saved in the lparam member of the TCITEM structure assigned to
this tab.

Tab icons are stored in a global image list (g_hImageList). They are loaded at
plugin startup, or after a icon change event. Multiple containers can share the
same image list, because the list is read-only.

$Id$
*/

#include "commonheaders.h"
#include "sendqueue.h"
#pragma hdrstop

#define SB_CHAR_WIDTH			45
#define DEFAULT_CONTAINER_POS 	0x00400040
#define DEFAULT_CONTAINER_SIZE 	0x019001f4

static HMONITOR(WINAPI * MyMonitorFromWindow)(HWND, DWORD) = 0;
static BOOL(WINAPI * MyGetMonitorInfoA)(HMONITOR, LPMONITORINFO) = 0;

extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;
extern HWND floaterOwner;
extern HANDLE hMessageWindowList;
extern HINSTANCE g_hInst;
extern SESSION_INFO *m_WndList;
extern BOOL (WINAPI *pfnIsThemeActive)();
extern HBRUSH g_MenuBGBrush;
extern int SIDEBARWIDTH;
extern ButtonSet g_ButtonSet;
extern int status_icon_list_size;
extern struct StatusIconListNode *status_icon_list;
extern      int g_sessionshutdown;

extern BOOL CALLBACK SelectContainerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK TabControlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcAbout(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcTemplateHelp(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

extern TCHAR *NewTitle(HANDLE hContact, const TCHAR *szFormat, const TCHAR *szNickname, const TCHAR *szStatus, const TCHAR *szContainer, const char *szUin, const char *szProto, DWORD idle, UINT codePage, BYTE xStatus, WORD wStatus);

// test

TCHAR 	*szWarnClose = _T("Do you really want to close this session?");
BOOL 	cntHelpActive = FALSE;
DWORD 	m_LangPackCP = CP_ACP;

struct 	ContainerWindowData *pFirstContainer = 0;        // the linked list of struct ContainerWindowData
struct 	ContainerWindowData *pLastActiveContainer = NULL;

TCHAR 	*MY_DBGetContactSettingString(HANDLE hContact, char *szModule, char *szSetting);

static 	WNDPROC OldStatusBarproc = 0, OldContainerWndProc = 0;

extern 	StatusItems_t StatusItems[];
BOOL 	g_skinnedContainers = FALSE;				// message containers are skinned
BOOL 	g_framelessSkinmode = FALSE;				// when this is true, tabSRMM will also draw the window borders and frame.
													// this indicates "fully skinned" mode. Otherwise, window border and frame
													// is using standard windows visuals.

extern HBRUSH	g_ContainerColorKeyBrush;
extern COLORREF g_ContainerColorKey;
extern SIZE		g_titleBarButtonSize;
extern int		g_titleButtonTopOff,g_titleBarLeftOff,g_titleBarRightOff, g_captionOffset, g_captionPadding, g_sidebarTopOffset, g_sidebarBottomOffset;

static BOOL	CALLBACK ContainerWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

static TCHAR	*menuBarNames[] = {_T("File"), _T("View"), _T("Message Log"), _T("Container"), _T("Help") };
static TCHAR	*menuBarNames_translated[6];
static BOOL		menuBarNames_done = FALSE;


#define RWPF_HIDE 8

/*
 * helper functions to restory container window position
 * taken from the core, but modified
 */
static int MY_RestoreWindowPosition(WPARAM wParam, LPARAM lParam)
{
	SAVEWINDOWPOS *swp = (SAVEWINDOWPOS*)lParam;
	WINDOWPLACEMENT wp;
	char szSettingName[64];
	int x, y;

	wp.length = sizeof(wp);
	GetWindowPlacement(swp->hwnd, &wp);
	wsprintfA(szSettingName, "%sx", swp->szNamePrefix);
	x = DBGetContactSettingDword(swp->hContact, swp->szModule, szSettingName, -1);
	wsprintfA(szSettingName, "%sy", swp->szNamePrefix);
	y = (int)DBGetContactSettingDword(swp->hContact, swp->szModule, szSettingName, -1);
	if (x == -1)
		return 1;
	if (wParam&RWPF_NOSIZE) {
		OffsetRect(&wp.rcNormalPosition, x - wp.rcNormalPosition.left, y - wp.rcNormalPosition.top);
	}
	else {
		wp.rcNormalPosition.left = x;
		wp.rcNormalPosition.top = y;
		wsprintfA(szSettingName, "%swidth", swp->szNamePrefix);
		wp.rcNormalPosition.right = wp.rcNormalPosition.left + DBGetContactSettingDword(swp->hContact, swp->szModule, szSettingName, -1);
		wsprintfA(szSettingName, "%sheight", swp->szNamePrefix);
		wp.rcNormalPosition.bottom = wp.rcNormalPosition.top + DBGetContactSettingDword(swp->hContact, swp->szModule, szSettingName, -1);
	}
	wp.flags = 0;
	if (wParam&RWPF_NOACTIVATE)
		wp.showCmd = SW_SHOWNOACTIVATE;
	if(DBGetContactSettingByte(swp->hContact, swp->szModule, "splitmax", 0))
		wp.showCmd = SW_SHOWMAXIMIZED;
	if (wParam & RWPF_HIDE)
		wp.showCmd = SW_HIDE;

	SetWindowPlacement(swp->hwnd, &wp);
	return 0;
}

static int MY_Utils_RestoreWindowPosition(HWND hwnd, HANDLE hContact, const char *szModule, const char *szNamePrefix, WPARAM wParam)
{
	SAVEWINDOWPOS swp;
	swp.hwnd = hwnd;
	swp.hContact = hContact;
	swp.szModule = szModule;
	swp.szNamePrefix = szNamePrefix;
	return MY_RestoreWindowPosition(wParam, (LPARAM)&swp);
}
static int MY_Utils_RestoreWindowPositionNoSize(HWND hwnd, HANDLE hContact, const char *szModule, const char *szNamePrefix, WPARAM wParam)
{
	SAVEWINDOWPOS swp;
	swp.hwnd = hwnd;
	swp.hContact = hContact;
	swp.szModule = szModule;
	swp.szNamePrefix = szNamePrefix;
	return MY_RestoreWindowPosition(wParam | RWPF_NOSIZE, (LPARAM)&swp);
}
static int MY_Utils_RestoreWindowPositionNoMove(HWND hwnd, HANDLE hContact, const char *szModule, const char *szNamePrefix, WPARAM wParam)
{
	SAVEWINDOWPOS swp;
	swp.hwnd = hwnd;
	swp.hContact = hContact;
	swp.szModule = szModule;
	swp.szNamePrefix = szNamePrefix;
	return MY_RestoreWindowPosition(wParam | RWPF_NOMOVE, (LPARAM)&swp);
}

/*
 * CreateContainer MUST malloc() a struct ContainerWindowData and pass its address
 * to CreateDialogParam() via the LPARAM. It also adds the struct to the linked list
 * of containers.
 *
 * The WM_DESTROY handler of the container DlgProc is responsible for free()'ing the
 * pointer and for removing the struct from the linked list.
 */

extern HMODULE hDLL;
extern PSLWA pSetLayeredWindowAttributes;
extern PULW pUpdateLayeredWindow;
extern PFWEX MyFlashWindowEx;

static int ServiceParamsOK(ButtonItem *item, WPARAM *wParam, LPARAM *lParam, HANDLE hContact)
{
	if (item->dwFlags & BUTTON_PASSHCONTACTW || item->dwFlags & BUTTON_PASSHCONTACTL || item->dwFlags & BUTTON_ISCONTACTDBACTION) {
		if (hContact == 0)
			return 0;
		if (item->dwFlags & BUTTON_PASSHCONTACTW)
			*wParam = (WPARAM)hContact;
		else if (item->dwFlags & BUTTON_PASSHCONTACTL)
			*lParam = (LPARAM)hContact;
		return 1;
	}
	return 1;                                       // doesn't need a paramter
}

struct ContainerWindowData *CreateContainer(const TCHAR *name, int iTemp, HANDLE hContactFrom) {
	DBVARIANT dbv;
	char szCounter[10];
#if defined (_UNICODE)
	char *szKey = "TAB_ContainersW";
#else
	char *szKey = "TAB_Containers";
#endif
	int i, iFirstFree = -1, iFound = FALSE;

	struct ContainerWindowData *pContainer = (struct ContainerWindowData *)malloc(sizeof(struct ContainerWindowData));
	if (pContainer) {
		ZeroMemory((void *)pContainer, sizeof(struct ContainerWindowData));
		_tcsncpy(pContainer->szName, name, CONTAINER_NAMELEN + 1);
		AppendToContainerList(pContainer);

		if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "limittabs", 0) && !_tcscmp(name, _T("default")))
			iTemp |= CNT_CREATEFLAG_CLONED;
		/*
		 * save container name to the db
		 */
		i = 0;
		if (!DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0)) {
			do {
				_snprintf(szCounter, 8, "%d", i);
				if (DBGetContactSettingTString(NULL, szKey, szCounter, &dbv)) {
					if (iFirstFree != -1) {
						pContainer->iContainerIndex = iFirstFree;
						_snprintf(szCounter, 8, "%d", iFirstFree);
					}
					else {
						pContainer->iContainerIndex = i;
					}
					DBWriteContactSettingTString(NULL, szKey, szCounter, name);
					BuildContainerMenu();
					break;
				}
				else {
					if (dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_WCHAR) {
						if (!_tcsncmp(dbv.ptszVal, name, CONTAINER_NAMELEN)) {
							pContainer->iContainerIndex = i;
							iFound = TRUE;
						}
						else if (!_tcsncmp(dbv.ptszVal, _T("**free**"), CONTAINER_NAMELEN))
							iFirstFree =  i;
					}
					DBFreeVariant(&dbv);
				}
			}
			while (++i && iFound == FALSE);
		}
		else {
			iTemp |= CNT_CREATEFLAG_CLONED;
			pContainer->iContainerIndex = 1;
		}

		if (iTemp & CNT_CREATEFLAG_MINIMIZED)
			pContainer->dwFlags = CNT_CREATE_MINIMIZED;
		if (iTemp & CNT_CREATEFLAG_CLONED) {
			pContainer->dwFlags |= CNT_CREATE_CLONED;
			pContainer->hContactFrom = hContactFrom;
		}
		pContainer->hwnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MSGCONTAINER), NULL, DlgProcContainer, (LPARAM) pContainer);
		return pContainer;
	}
	return NULL;
}

static int tooltip_active = FALSE;
static POINT ptMouse = {0};
RECT 	rcLastStatusBarClick;						// remembers click (down event) point for status bar clicks

LRESULT CALLBACK StatusBarSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct ContainerWindowData *pContainer = (struct ContainerWindowData *)GetWindowLong(GetParent(hWnd), GWL_USERDATA);

	if (!OldStatusBarproc) {
		WNDCLASSEX wc = {0};
		wc.cbSize = sizeof(wc);
		GetClassInfoEx(g_hInst, STATUSCLASSNAME, &wc);
		OldStatusBarproc = wc.lpfnWndProc;
	}
	switch (msg) {
		case WM_CREATE: {
			CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
			LRESULT ret;
			HWND hwndParent = GetParent(hWnd);
			SetWindowLong(hwndParent, GWL_STYLE, GetWindowLong(hwndParent, GWL_STYLE) & ~WS_THICKFRAME);
			SetWindowLong(hwndParent, GWL_EXSTYLE, GetWindowLong(hwndParent, GWL_EXSTYLE) & ~WS_EX_APPWINDOW);
			cs->style &= ~SBARS_SIZEGRIP;
			ret = CallWindowProc(OldStatusBarproc, hWnd, msg, wParam, lParam);
			SetWindowLong(hwndParent, GWL_STYLE, GetWindowLong(hwndParent, GWL_STYLE) | WS_THICKFRAME);
			SetWindowLong(hwndParent, GWL_EXSTYLE, GetWindowLong(hwndParent, GWL_EXSTYLE) | WS_EX_APPWINDOW);
			return ret;
		}
		case WM_COMMAND:
			if (LOWORD(wParam) == 1001)
				SendMessage(GetParent(hWnd), DM_SESSIONLIST, 0, 0);
			break;
		case WM_NCHITTEST: {
			RECT r;
			POINT pt;
			LRESULT lr = SendMessage(GetParent(hWnd), WM_NCHITTEST, wParam, lParam);
			int clip = myGlobals.bClipBorder;

			GetWindowRect(hWnd, &r);
			GetCursorPos(&pt);
			if (pt.y <= r.bottom && pt.y >= r.bottom - clip - 3) {
				if (pt.x > r.right - clip - 4)
					return HTBOTTOMRIGHT;
			}
			if (lr == HTLEFT || lr == HTRIGHT || lr == HTBOTTOM || lr == HTTOP || lr == HTTOPLEFT || lr == HTTOPRIGHT
					|| lr == HTBOTTOMLEFT || lr == HTBOTTOMRIGHT)
				return HTTRANSPARENT;
			break;
		}
		case WM_ERASEBKGND: {
			RECT rcClient;
			if (pContainer && pContainer->bSkinned)
				return 1;
			if (pfnIsThemeActive) {
				if (pfnIsThemeActive())
					break;
			}
			GetClientRect(hWnd, &rcClient);
			FillRect((HDC)wParam, &rcClient, GetSysColorBrush(COLOR_3DFACE));
			return 1;
		}
		case WM_PAINT:
			if (!g_skinnedContainers)
				break;
			else {
				PAINTSTRUCT ps;
				TCHAR szText[1024];
				int i;
				RECT itemRect;
				HICON hIcon;
				LONG height, width;
				HDC hdc = BeginPaint(hWnd, &ps);
				HFONT hFontOld = 0;
				UINT nParts = SendMessage(hWnd, SB_GETPARTS, 0, 0);
				LRESULT result;
				RECT rcClient;
				HDC hdcMem = CreateCompatibleDC(hdc);
				HBITMAP hbm, hbmOld;
				StatusItems_t *item = &StatusItems[ID_EXTBKSTATUSBARPANEL];

				GetClientRect(hWnd, &rcClient);

				hbm = CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
				hbmOld = SelectObject(hdcMem, hbm);
				SetBkMode(hdcMem, TRANSPARENT);
				SetTextColor(hdcMem, myGlobals.skinDefaultFontColor);
				hFontOld = SelectObject(hdcMem, GetStockObject(DEFAULT_GUI_FONT));

				if (pContainer && pContainer->bSkinned)
					SkinDrawBG(hWnd, GetParent(hWnd), pContainer, &rcClient, hdcMem);

				for (i = 0; i < (int)nParts; i++) {
					SendMessage(hWnd, SB_GETRECT, (WPARAM)i, (LPARAM)&itemRect);
					if (!item->IGNORED)
						DrawAlpha(hdcMem, &itemRect, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT, item->GRADIENT,
								  item->CORNER, item->BORDERSTYLE, item->imageItem);

					if (i == 0)
						itemRect.left += 2;

					height = itemRect.bottom - itemRect.top;
					width = itemRect.right - itemRect.left;
					hIcon = (HICON)SendMessage(hWnd, SB_GETICON, i, 0);
					szText[0] = 0;
					result = SendMessage(hWnd, SB_GETTEXT, i, (LPARAM)szText);
					if (i == 2 && pContainer) {
						struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);

						if (dat)
							DrawStatusIcons(dat, hdcMem, itemRect, 2);
					}
					else {
						if (hIcon) {
							if (LOWORD(result) > 1) {				// we have a text
								DrawIconEx(hdcMem, itemRect.left + 3, (height / 2 - 8) + itemRect.top, hIcon, 16, 16, 0, 0, DI_NORMAL);
								itemRect.left += 20;
								DrawText(hdcMem, szText, -1, &itemRect, DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE | DT_NOPREFIX);
							}
							else
								DrawIconEx(hdcMem, itemRect.left + 3, (height / 2 - 8) + itemRect.top, hIcon, 16, 16, 0, 0, DI_NORMAL);
						}
						else {
							itemRect.left += 2;
							itemRect.right -= 2;
							DrawText(hdcMem, szText, -1, &itemRect, DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
						}
					}
				}
				BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);
				SelectObject(hdcMem, hbmOld);
				DeleteObject(hbm);
				SelectObject(hdcMem, hFontOld);
				DeleteDC(hdcMem);

				EndPaint(hWnd, &ps);
				return 0;
			}
		case WM_CONTEXTMENU: {
			RECT rc;
			POINT pt;
			GetCursorPos(&pt);
			GetWindowRect(GetDlgItem(hWnd, 1001), &rc);
			if (PtInRect(&rc, pt)) {
				SendMessage(myGlobals.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 100, WM_RBUTTONUP);
				return 0;
			}
			break;
		}

		case WM_USER + 101: {
			struct MessageWindowData *dat = (struct MessageWindowData *)lParam;
			RECT rcs;
			int statwidths[5];
			struct StatusIconListNode *current = status_icon_list;
			int    list_icons = 0;
			char   buff[100];
			DWORD  flags;

			while (current && dat) {
				flags=current->sid.flags;
				if(current->sid.flags&MBF_OWNERSTATE){
					struct StatusIconListNode *currentSIN = dat->pSINod;

					while (currentSIN) {
						if (strcmp(currentSIN->sid.szModule, current->sid.szModule) == 0 && currentSIN->sid.dwId == current->sid.dwId) {
							flags=currentSIN->sid.flags;
							break;
						}
						currentSIN = currentSIN->next;
					}
				}
				else {
					sprintf(buff, "SRMMStatusIconFlags%d", (int)current->sid.dwId);
					flags = DBGetContactSettingByte(dat->hContact, current->sid.szModule, buff, current->sid.flags);
				}
				if (!(flags & MBF_HIDDEN))
					list_icons++;
				current = current->next;
			}

			SendMessage(hWnd, WM_SIZE, 0, 0);
			GetWindowRect(hWnd, &rcs);

			statwidths[0] = (rcs.right - rcs.left) - (2 * SB_CHAR_WIDTH + 20) - (35 + ((list_icons) * (myGlobals.m_smcxicon + 2)));
			statwidths[1] = (rcs.right - rcs.left) - (45 + ((list_icons) * (myGlobals.m_smcxicon + 2)));
			statwidths[2] = -1;
			SendMessage(hWnd, SB_SETPARTS, 3, (LPARAM) statwidths);
			return 0;
		}

		case WM_SETCURSOR: {
			POINT pt;

			GetCursorPos(&pt);
			SendMessage(GetParent(hWnd), msg, wParam, lParam);
			if (pt.x == ptMouse.x && pt.y == ptMouse.y) {
				return 1;
			}
			ptMouse = pt;
			if (tooltip_active) {
				KillTimer(hWnd, TIMERID_HOVER);
				CallService("mToolTip/HideTip", 0, 0);
				tooltip_active = FALSE;
			}
			KillTimer(hWnd, TIMERID_HOVER);
			SetTimer(hWnd, TIMERID_HOVER, 450, 0);
			break;
		}

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN: {
			POINT 	pt;

			KillTimer(hWnd, TIMERID_HOVER);
			CallService("mToolTip/HideTip", 0, 0);
			tooltip_active = FALSE;
			GetCursorPos(&pt);
			rcLastStatusBarClick.left = pt.x - 2;
			rcLastStatusBarClick.right = pt.x + 2;
			rcLastStatusBarClick.top = pt.y - 2;
			rcLastStatusBarClick.bottom = pt.y + 2;
			break;
		}

		case WM_TIMER:
			if (wParam == TIMERID_HOVER) {
				POINT pt;
				BOOL  fHaveUCTip = ServiceExists("mToolTip/ShowTipW");
				CLCINFOTIP ti = {0};
				ti.cbSize = sizeof(ti);

				KillTimer(hWnd, TIMERID_HOVER);
				GetCursorPos(&pt);
				if (pt.x == ptMouse.x && pt.y == ptMouse.y) {
					RECT rc;
					struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);
//mad
					SIZE size;
					TCHAR  szStatusBarText[512];
//mad_
					ti.ptCursor = pt;
					ScreenToClient(hWnd, &pt);
					SendMessage(hWnd, SB_GETRECT, 2, (LPARAM)&rc);
					if (dat && PtInRect(&rc, pt)) {
						int gap = 2;
						struct StatusIconListNode *current = status_icon_list;
						struct StatusIconListNode *clicked = NULL;
						struct StatusIconListNode *currentSIN = NULL;

						unsigned int iconNum = (pt.x - rc.left) / (myGlobals.m_smcxicon + gap);
						unsigned int list_icons = 0;
						char         buff[100];
						DWORD        flags;

						while (current) {
							if(current->sid.flags&MBF_OWNERSTATE&&dat->pSINod){
								struct StatusIconListNode *currentSIN = dat->pSINod;
								flags=current->sid.flags;
								while (currentSIN)
									{
									if (strcmp(currentSIN->sid.szModule, current->sid.szModule) == 0 && currentSIN->sid.dwId == current->sid.dwId) {
										flags=currentSIN->sid.flags;
										break;
										}
									currentSIN = currentSIN->next;
									}
								}
							else  {
							sprintf(buff, "SRMMStatusIconFlags%d", (int)current->sid.dwId);
							flags = DBGetContactSettingByte(dat->hContact, current->sid.szModule, buff, current->sid.flags);
							}
							if (!(flags & MBF_HIDDEN)) {
								if (list_icons++ == iconNum)
									clicked = current;
							}
							current = current->next;
						}

 						if(clicked&&clicked->sid.flags&MBF_OWNERSTATE){
 							currentSIN=dat->pSINod;
 							while (currentSIN)
 								{
 								if (strcmp(currentSIN->sid.szModule, clicked->sid.szModule) == 0 && currentSIN->sid.dwId == clicked->sid.dwId) {
 									clicked=currentSIN;
 									break;
 									}
 								currentSIN = currentSIN->next;
 								}
 							}

						if ((int)iconNum == list_icons && pContainer) {
#if defined(_UNICODE)
							if (fHaveUCTip) {
								wchar_t wBuf[512];

								mir_sntprintf(wBuf, safe_sizeof(wBuf), TranslateT("Sounds are %s. Click to toggle status, hold SHIFT and click to set for all open containers"), pContainer->dwFlags & CNT_NOSOUND ? TranslateT("disabled") : TranslateT("enabled"));
								CallService("mToolTip/ShowTipW", (WPARAM)wBuf, (LPARAM)&ti);
							}
							else {
								char buf[512];

								mir_snprintf(buf, sizeof(buf), Translate("Sounds are %s. Click to toggle status, hold SHIFT and click to set for all open containers"), pContainer->dwFlags & CNT_NOSOUND ? Translate("disabled") : Translate("enabled"));
								CallService("mToolTip/ShowTip", (WPARAM)buf, (LPARAM)&ti);
							}
#else
							char buf[512];
							mir_snprintf(buf, sizeof(buf), Translate("Sounds are %s. Click to toggle status, hold SHIFT and click to set for all open containers"), pContainer->dwFlags & CNT_NOSOUND ? Translate("disabled") : Translate("enabled"));
							CallService("mToolTip/ShowTip", (WPARAM)buf, (LPARAM)&ti);
#endif
							tooltip_active = TRUE;
						}
						else if ((int)iconNum == list_icons + 1 && dat && dat->bType == SESSIONTYPE_IM) {
							int mtnStatus = (int)DBGetContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW));
#if defined(_UNICODE)

							if (fHaveUCTip) {
								wchar_t wBuf[512];

								mir_sntprintf(wBuf, safe_sizeof(wBuf), TranslateT("Sending typing notifications is %s."), mtnStatus ? TranslateT("enabled") : TranslateT("disabled"));
								CallService("mToolTip/ShowTipW", (WPARAM)wBuf, (LPARAM)&ti);
							}
							else {
								char buf[512];

								mir_snprintf(buf, sizeof(buf), Translate("Sending typing notifications is %s."), mtnStatus ? Translate("enabled") : Translate("disabled"));
								CallService("mToolTip/ShowTip", (WPARAM)buf, (LPARAM)&ti);
							}
#else
							char buf[512];
							mir_snprintf(buf, sizeof(buf), Translate("Sending typing notifications is %s."), mtnStatus ? Translate("enabled") : Translate("disabled"));
							CallService("mToolTip/ShowTip", (WPARAM)buf, (LPARAM)&ti);
#endif
							tooltip_active = TRUE;
						}
						else {
							if (clicked) {
								CallService("mToolTip/ShowTip", (WPARAM)clicked->sid.szTooltip, (LPARAM)&ti);
								tooltip_active = TRUE;
							}
						}
					}
					SendMessage(hWnd, SB_GETRECT, 1, (LPARAM)&rc);
					if (dat && PtInRect(&rc, pt)) {
						int iLength = 0;
#if defined(_UNICODE)
						GETTEXTLENGTHEX gtxl = {0};

						gtxl.codepage = CP_UTF8;
						gtxl.flags = GTL_DEFAULT | GTL_PRECISE | GTL_NUMBYTES;
						iLength = SendDlgItemMessage(dat->hwnd, dat->bType == SESSIONTYPE_IM ? IDC_MESSAGE : IDC_CHAT_MESSAGE, EM_GETTEXTLENGTHEX, (WPARAM) & gtxl, 0);
						tooltip_active = TRUE;
						if (fHaveUCTip) {
							wchar_t wBuf[512];
							wchar_t *szFormat = _T("There are %d pending send jobs. Message length: %d bytes, message length limit: %d bytes");

							mir_sntprintf(wBuf, safe_sizeof(wBuf), TranslateTS(szFormat), dat->iOpenJobs, iLength, dat->nMax ? dat->nMax : 20000);
							CallService("mToolTip/ShowTipW", (WPARAM)wBuf, (LPARAM)&ti);
						}
						else {
							char buf[512];
							char *szFormat = "There are %d pending send jobs. Message length: %d bytes, message length limit: %d bytes";

							mir_snprintf(buf, sizeof(buf), Translate(szFormat), dat->iOpenJobs, iLength, dat->nMax ? dat->nMax : 20000);
							CallService("mToolTip/ShowTip", (WPARAM)buf, (LPARAM)&ti);
						}
#else
						char buf[512];
						char *szFormat = "There are %d pending send jobs. Message length: %d bytes, message length limit: %d bytes";
 						iLength = GetWindowTextLength(GetDlgItem(dat->hwnd, dat->bType == SESSIONTYPE_IM ? IDC_MESSAGE : IDC_CHAT_MESSAGE));
 						mir_snprintf(buf, sizeof(buf), Translate(szFormat), dat->iOpenJobs, iLength, dat->nMax ? dat->nMax : 20000);
						CallService("mToolTip/ShowTip", (WPARAM)buf, (LPARAM)&ti);
#endif
					}
					//MAD
					if(SendMessage(dat->pContainer->hwndStatus, SB_GETTEXT, 0, (LPARAM)szStatusBarText))
					{
						HDC hdc;
						int iLen=SendMessage(dat->pContainer->hwndStatus,SB_GETTEXTLENGTH,0,0);
						SendMessage(hWnd, SB_GETRECT, 0, (LPARAM)&rc);
						GetTextExtentPoint32( hdc=GetDC( dat->pContainer->hwndStatus), szStatusBarText, iLen, &size );
						ReleaseDC (dat->pContainer->hwndStatus,hdc);

					if(dat && PtInRect(&rc,pt)&&((rc.right-rc.left)<size.cx)) {
						DBVARIANT dbv={0};

						if(dat->bType == SESSIONTYPE_CHAT)
							DBGetContactSettingTString(dat->hContact,dat->szProto,"Topic",&dbv);

						tooltip_active = TRUE;
#if defined(_UNICODE)
						if(fHaveUCTip)	 CallService("mToolTip/ShowTipW", (WPARAM)(dbv.pwszVal?dbv.pwszVal:szStatusBarText), (LPARAM)&ti);
						else CallService("mToolTip/ShowTip", (WPARAM)mir_u2a(dbv.pwszVal?dbv.pwszVal:szStatusBarText), (LPARAM)&ti);
#else
						CallService("mToolTip/ShowTip", (WPARAM)(dbv.pszVal?dbv.pszVal:szStatusBarText), (LPARAM)&ti);
#endif
					if(dbv.pszVal) DBFreeVariant(&dbv);
					}
				}
					// MAD_
				}
			}
			break;
		case WM_DESTROY:
			KillTimer(hWnd, TIMERID_HOVER);
	}
	return CallWindowProc(OldStatusBarproc, hWnd, msg, wParam, lParam);
}
/*
 * draws the sidebar, arranges the buttons, cares about hiding buttons if space
 * is not sufficient.
 * also, updates the scroll buttons on top and bottom of the sidebar
 */

static void DrawSideBar(HWND hwndDlg, struct ContainerWindowData *pContainer, RECT *rc)
{
	int i, topCount = 0, bottomCount = 0, j = 0;
	BOOL topEnabled = FALSE, bottomEnabled = FALSE;
	int leftMargin = pContainer->tBorder_outer_left;
	ButtonItem *pItem = pContainer->buttonItems;
	HWND hwnd;
	LONG topOffset = (g_sidebarTopOffset == -1 ? pContainer->tBorder_outer_top + 12 : g_sidebarTopOffset + 12);
	LONG topOffset_s = topOffset;
	LONG bottomOffset = 12 + pContainer->statusBarHeight + (g_sidebarBottomOffset == -1 ? pContainer->tBorder_outer_bottom : g_sidebarBottomOffset);
	LONG bottomOffset_s = bottomOffset;
	int iSpaceAvail = (rc->bottom - rc->top) - (topOffset + bottomOffset);
	int iTopSpaceAvail = iSpaceAvail - (pContainer->sb_BottomHeight + 4);
	BOOL hideFollowing = FALSE;
	DWORD dwFlags = SWP_NOZORDER | SWP_NOCOPYBITS;

	if (pContainer->sb_TopHeight <= iTopSpaceAvail)
		i = pContainer->sb_FirstButton = 0;
	else {
		if (pContainer->sb_FirstButton == 0)
			i = 0;
		else
			i = pContainer->sb_FirstButton;
	}
	while (pItem) {
		if (pItem->dwFlags & BUTTON_ISSIDEBAR) {
			if (pContainer->dwFlags & CNT_SIDEBAR) {
				hwnd = GetDlgItem(hwndDlg, pItem->uId);
				if (j < i) {
					ShowWindow(hwnd, SW_HIDE);
					j++;
					pItem = pItem->nextItem;
					continue;
				}
				if (pItem->yOff >= 0) {
					if (iTopSpaceAvail >= pItem->height && !hideFollowing) {
						SetWindowPos(hwnd, 0, leftMargin + ((SIDEBARWIDTH - pItem->width) / 2), topOffset, pItem->width, pItem->height, SWP_SHOWWINDOW | dwFlags);
						iTopSpaceAvail -= (pItem->height + 2);
						topOffset += (pItem->height + 2);
					}
					else {
						hideFollowing = TRUE;
						ShowWindow(hwnd, SW_HIDE);
						bottomEnabled = TRUE;
						iTopSpaceAvail -= (pItem->height + 2);
					}
				}
				else {
					SetWindowPos(hwnd, 0, leftMargin + ((SIDEBARWIDTH - pItem->width) / 2), (rc->bottom - rc->top) - bottomOffset - pItem->height, pItem->width, pItem->height, dwFlags);
					bottomOffset += (pItem->height + 2);
				}
			}
			j++;
		}
		else {
			LONG xOffset = pItem->xOff >= 0 ? pItem->xOff : rc->right - abs(pItem->xOff);
			LONG yOffset = pItem->yOff >= 0 ? pItem->yOff : rc->bottom - abs(pItem->yOff) - pContainer->statusBarHeight;
			if (g_sidebarTopOffset != -1 && pItem->xOff >= 0 && pItem->yOff >= 0)
				xOffset += (pContainer->dwFlags & CNT_SIDEBAR ? SIDEBARWIDTH + 2 : 0);
			if (g_sidebarBottomOffset != -1 && pItem->xOff >= 0 && pItem->yOff < 0)
				xOffset += (pContainer->dwFlags & CNT_SIDEBAR ? SIDEBARWIDTH + 2 : 0);
			hwnd = GetDlgItem(hwndDlg, pItem->uId);
			SetWindowPos(hwnd, 0, xOffset, yOffset, pItem->width, pItem->height, SWP_SHOWWINDOW | dwFlags);
		}
		pItem = pItem->nextItem;
	}
	topEnabled = pContainer->sb_FirstButton ? TRUE : FALSE;
	MoveWindow(GetDlgItem(hwndDlg, IDC_SIDEBARUP), 2 + leftMargin, topOffset_s - 12, SIDEBARWIDTH - 4, 10, TRUE);
	MoveWindow(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN), 2 + leftMargin, (rc->bottom - rc->top) - bottomOffset_s + 2, SIDEBARWIDTH - 4, 10, TRUE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_SIDEBARUP), topEnabled);
	EnableWindow(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN), bottomEnabled);
	InvalidateRect(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN), NULL, FALSE);
	InvalidateRect(GetDlgItem(hwndDlg, IDC_SIDEBARUP), NULL, FALSE);
}

static BOOL CALLBACK ContainerWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL bSkinned;

	struct ContainerWindowData *pContainer = (struct ContainerWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);
	bSkinned = pContainer ? pContainer->bSkinned : FALSE;

	switch (msg) {
		case WM_NCLBUTTONDOWN:
		case WM_NCLBUTTONUP:
		case WM_NCMOUSEHOVER:
		case WM_NCMOUSEMOVE:
			if (pContainer && g_framelessSkinmode) {
				POINT pt;
				RECT rcWindow;
				BOOL isMin, isMax, isClose;
				int i;

				GetCursorPos(&pt);
				GetWindowRect(hwndDlg, &rcWindow);

				CopyMemory(&pContainer->oldbuttons[0], &pContainer->buttons[0], sizeof(struct TitleBtn) * 3);
				ZeroMemory(&pContainer->buttons[0], sizeof(struct TitleBtn) * 3);
				isMin = isMax = isClose = FALSE;

				if (pt.x >= (rcWindow.left + pContainer->rcMin.left) && pt.x <= (rcWindow.left + pContainer->rcClose.right) && pt.y < rcWindow.top + 24 && wParam != HTTOPRIGHT) {
					LRESULT result = 0; //DefWindowProc(hwndDlg, msg, wParam, lParam);
					HDC hdc = GetWindowDC(hwndDlg);
					LONG left = rcWindow.left;

					pt.y = 10;
					isMin = pt.x >= left + pContainer->rcMin.left && pt.x <= left + pContainer->rcMin.right;
					isMax = pt.x >= left + pContainer->rcMax.left && pt.x <= left + pContainer->rcMax.right;
					isClose = pt.x >= left + pContainer->rcClose.left && pt.x <= left + pContainer->rcClose.right;

					if (msg == WM_NCMOUSEMOVE) {
						if (isMax)
							pContainer->buttons[BTN_MAX].isHot = TRUE;
						else if (isMin)
							pContainer->buttons[BTN_MIN].isHot = TRUE;
						else if (isClose)
							pContainer->buttons[BTN_CLOSE].isHot = TRUE;
					}
					else if (msg == WM_NCLBUTTONDOWN) {
						if (isMax)
							pContainer->buttons[BTN_MAX].isPressed = TRUE;
						else if (isMin)
							pContainer->buttons[BTN_MIN].isPressed = TRUE;
						else if (isClose)
							pContainer->buttons[BTN_CLOSE].isPressed = TRUE;
					}
					else if (msg == WM_NCLBUTTONUP) {
						if (isMin)
							PostMessage(hwndDlg, WM_SYSCOMMAND, SC_MINIMIZE, 0);
						else if (isMax) {
							if (IsZoomed(hwndDlg))
								PostMessage(hwndDlg, WM_SYSCOMMAND, SC_RESTORE, 0);
							else
								PostMessage(hwndDlg, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
						}
						else if (isClose)
							PostMessage(hwndDlg, WM_SYSCOMMAND, SC_CLOSE, 0);
					}
					for (i = 0; i < 3; i++) {
						if (pContainer->buttons[i].isHot != pContainer->oldbuttons[i].isHot) {
							RECT *rc = 0;
							HICON hIcon;

							switch (i) {
								case 0:
									rc = &pContainer->rcMin;
									hIcon = myGlobals.g_minGlyph;
									break;
								case 1:
									rc = &pContainer->rcMax;
									hIcon = myGlobals.g_maxGlyph;
									break;
								case 2:
									rc = &pContainer->rcClose;
									hIcon = myGlobals.g_closeGlyph;
									break;
							}
							if (rc) {
								StatusItems_t *item = &StatusItems[pContainer->buttons[i].isPressed ? ID_EXTBKTITLEBUTTONPRESSED : (pContainer->buttons[i].isHot ? ID_EXTBKTITLEBUTTONMOUSEOVER : ID_EXTBKTITLEBUTTON)];
								DrawAlpha(hdc, rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
										  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
								DrawIconEx(hdc, rc->left + ((rc->right - rc->left) / 2 - 8), rc->top + ((rc->bottom - rc->top) / 2 - 8), hIcon, 16, 16, 0, 0, DI_NORMAL);
							}
						}
					}
					ReleaseDC(hwndDlg, hdc);
					return result;
				}
				else {
					LRESULT result = DefWindowProc(hwndDlg, msg, wParam, lParam);
					RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_NOCHILDREN);
					return result;
				}
			}
			break;
		case WM_SETCURSOR: {
			if (g_framelessSkinmode && (HWND)wParam == hwndDlg) {
				DefWindowProc(hwndDlg, msg, wParam, lParam);
				RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_NOCHILDREN);
				return 1;
			}
			break;
		}
		case WM_NCCALCSIZE: {
			if (!g_framelessSkinmode)
				break;

			if (wParam) {
				RECT *rc;
				NCCALCSIZE_PARAMS *ncsp = (NCCALCSIZE_PARAMS *)lParam;

				DefWindowProc(hwndDlg, msg, wParam, lParam);
				rc = &ncsp->rgrc[0];

				rc->left += myGlobals.g_realSkinnedFrame_left;
				rc->right -= myGlobals.g_realSkinnedFrame_right;
				rc->bottom -= myGlobals.g_realSkinnedFrame_bottom;
				rc->top += myGlobals.g_realSkinnedFrame_caption;
				return TRUE;
			}
			else {
				return DefWindowProc(hwndDlg, msg, wParam, lParam);
			}
		}
		case WM_NCACTIVATE:
			if (pContainer) {
				pContainer->ncActive = wParam;
				if (bSkinned && g_framelessSkinmode) {
					SendMessage(hwndDlg, WM_NCPAINT, 0, 0);
					return 1;
				}
			}
			break;
		case WM_NCPAINT: {
			PAINTSTRUCT ps;
			HDC hdcReal;
			RECT rcClient;
			LONG width, height;
			HDC hdc;
			StatusItems_t *item = &StatusItems[0], *item_normal, *item_pressed, *item_hot;
			HICON hIcon;
			HFONT hOldFont = 0;
			TEXTMETRIC tm;

			if (!pContainer || !bSkinned)
				break;

			if (g_framelessSkinmode) {
				RECT rcWindow, rcClient;
				HDC dcFrame = GetWindowDC(hwndDlg);
				POINT pt, pt1;
				LONG clip_top, clip_left;
				HRGN rgn = 0;
				StatusItems_t *item;
				TCHAR szWindowText[512];
				RECT rcText;
				HDC dcMem = CreateCompatibleDC(pContainer->cachedDC ? pContainer->cachedDC : dcFrame);
				HBITMAP hbmMem, hbmOld;
				int i;
				DRAWITEMSTRUCT dis = {0};

				GetWindowRect(hwndDlg, &rcWindow);
				GetClientRect(hwndDlg, &rcClient);
				pt.y = 0;
				pt.x = 0;
				ClientToScreen(hwndDlg, &pt);
				pt1.x = rcClient.right;
				pt1.y = rcClient.bottom;
				ClientToScreen(hwndDlg, &pt1);
				clip_top = pt.y - rcWindow.top;
				clip_left = pt.x - rcWindow.left;

				rcWindow.right = rcWindow.right - rcWindow.left;
				rcWindow.bottom = rcWindow.bottom - rcWindow.top;
				rcWindow.left = rcWindow.top = 0;

				hbmMem = CreateCompatibleBitmap(dcFrame, rcWindow.right, rcWindow.bottom);
				hbmOld = SelectObject(dcMem, hbmMem);

				ExcludeClipRect(dcFrame, clip_left, clip_top, clip_left + (pt1.x - pt.x), clip_top + (pt1.y - pt.y));
				ExcludeClipRect(dcMem, clip_left, clip_top, clip_left + (pt1.x - pt.x), clip_top + (pt1.y - pt.y));
				item = pContainer->ncActive ? &StatusItems[ID_EXTBKFRAME] : &StatusItems[ID_EXTBKFRAMEINACTIVE];
				if (!item->IGNORED)
					DrawAlpha(dcMem, &rcWindow, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
							  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);

				GetWindowText(hwndDlg, szWindowText, 512);
				szWindowText[511] = 0;
				hOldFont = SelectObject(dcMem, myGlobals.hFontCaption);
				GetTextMetrics(dcMem, &tm);
				SetTextColor(dcMem, myGlobals.ipConfig.isValid ? myGlobals.ipConfig.clrs[IPFONTCOUNT - 1] : GetSysColor(COLOR_CAPTIONTEXT));
				SetBkMode(dcMem, TRANSPARENT);
				rcText.left =20 + myGlobals.g_SkinnedFrame_left + myGlobals.bClipBorder + g_titleBarLeftOff;//26;
				rcText.right = rcWindow.right - 3 * g_titleBarButtonSize.cx - 11 - g_titleBarRightOff;
				rcText.top = g_captionOffset + myGlobals.bClipBorder;
				rcText.bottom = rcText.top + tm.tmHeight;
				rcText.left += g_captionPadding;
				DrawText(dcMem, szWindowText, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
				SelectObject(dcMem, hOldFont);
				/*
				 * icon
				 */

				hIcon = (HICON)SendMessage(hwndDlg, WM_GETICON, ICON_BIG, 0);
 				DrawIconEx(dcMem, 4 + myGlobals.g_SkinnedFrame_left + myGlobals.bClipBorder+g_titleBarLeftOff, rcText.top + (rcText.bottom - rcText.top) / 2 - 8, hIcon, 16, 16, 0, 0, DI_NORMAL);

				// title buttons;

				pContainer->rcClose.top = pContainer->rcMin.top = pContainer->rcMax.top = g_titleButtonTopOff;
				pContainer->rcClose.bottom = pContainer->rcMin.bottom = pContainer->rcMax.bottom = g_titleButtonTopOff + g_titleBarButtonSize.cy;

				pContainer->rcClose.right = rcWindow.right - 10 - g_titleBarRightOff;
				pContainer->rcClose.left = pContainer->rcClose.right - g_titleBarButtonSize.cx;

				pContainer->rcMax.right = pContainer->rcClose.left - 2;
				pContainer->rcMax.left = pContainer->rcMax.right - g_titleBarButtonSize.cx;

				pContainer->rcMin.right = pContainer->rcMax.left - 2;
				pContainer->rcMin.left = pContainer->rcMin.right - g_titleBarButtonSize.cx;

				item_normal = &StatusItems[ID_EXTBKTITLEBUTTON];
				item_hot = &StatusItems[ID_EXTBKTITLEBUTTONMOUSEOVER];
				item_pressed = &StatusItems[ID_EXTBKTITLEBUTTONPRESSED];

				for (i = 0; i < 3; i++) {
					RECT *rc = 0;
					HICON hIcon;

					switch (i) {
						case 0:
							rc = &pContainer->rcMin;
							hIcon = myGlobals.g_minGlyph;
							break;
						case 1:
							rc = &pContainer->rcMax;
							hIcon = myGlobals.g_maxGlyph;
							break;
						case 2:
							rc = &pContainer->rcClose;
							hIcon = myGlobals.g_closeGlyph;
							break;
					}
					if (rc) {
						item = pContainer->buttons[i].isPressed ? item_pressed : (pContainer->buttons[i].isHot ? item_hot : item_normal);
						DrawAlpha(dcMem, rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
								  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
						DrawIconEx(dcMem, rc->left + ((rc->right - rc->left) / 2 - 8), rc->top + ((rc->bottom - rc->top) / 2 - 8), hIcon, 16, 16, 0, 0, DI_NORMAL);
					}
				}
				SetBkMode(dcMem, TRANSPARENT);
				if (!(pContainer->dwFlags & CNT_NOMENUBAR) && pContainer->hMenu) {
					RECT rcItem;
					RECT rcWindow2;

					GetWindowRect(hwndDlg, &rcWindow2);

					dis.hDC = dcMem;
					dis.CtlType = ODT_MENU;
					for (i = 0; i <= 4; i++) {
						dis.itemID = 0xffff5000 + i;
						GetMenuItemRect(hwndDlg, pContainer->hMenu, i, &rcItem);

						dis.rcItem.left = rcItem.left - rcWindow2.left;
						dis.rcItem.top = rcItem.top - rcWindow2.top;

						dis.rcItem.right = dis.rcItem.left + (rcItem.right - rcItem.left);
						dis.rcItem.bottom = dis.rcItem.top + (rcItem.bottom - rcItem.top);
						SendMessage(hwndDlg, WM_DRAWITEM, 0, (LPARAM)&dis);
					}
				}
				BitBlt(dcFrame, 0, 0, rcWindow.right, rcWindow.bottom, dcMem, 0, 0, SRCCOPY);
				SelectObject(dcMem, hbmOld);
				DeleteObject(hbmMem);
				DeleteDC(dcMem);
				ReleaseDC(hwndDlg, dcFrame);
			}
			else
				CallWindowProc(OldContainerWndProc, hwndDlg, msg, wParam, lParam);

			hdcReal = BeginPaint(hwndDlg, &ps);

			GetClientRect(hwndDlg, &rcClient);
			width = rcClient.right - rcClient.left;
			height = rcClient.bottom - rcClient.top;
			if (width != pContainer->oldDCSize.cx || height != pContainer->oldDCSize.cy) {
				StatusItems_t *sbaritem = &StatusItems[ID_EXTBKSTATUSBAR];
				BOOL statusBarSkinnd = !(pContainer->dwFlags & CNT_NOSTATUSBAR) && !sbaritem->IGNORED;
				LONG sbarDelta = statusBarSkinnd ? pContainer->statusBarHeight : 0;

				pContainer->oldDCSize.cx = width;
				pContainer->oldDCSize.cy = height;

				if (pContainer->cachedDC) {
					SelectObject(pContainer->cachedDC, pContainer->oldHBM);
					DeleteObject(pContainer->cachedHBM);
					DeleteDC(pContainer->cachedDC);
				}
				pContainer->cachedDC = CreateCompatibleDC(hdcReal);
				pContainer->cachedHBM = CreateCompatibleBitmap(hdcReal, width, height);
				pContainer->oldHBM = SelectObject(pContainer->cachedDC, pContainer->cachedHBM);

				hdc = pContainer->cachedDC;
				if (!item->IGNORED)
					DrawAlpha(hdc, &rcClient, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT,
							  item->GRADIENT, item->CORNER, item->BORDERSTYLE, item->imageItem);
				else
					FillRect(hdc, &rcClient, GetSysColorBrush(COLOR_3DFACE));

				if (sbarDelta) {
					rcClient.top = rcClient.bottom - sbarDelta;
					DrawAlpha(hdc, &rcClient, sbaritem->COLOR, sbaritem->ALPHA, sbaritem->COLOR2, sbaritem->COLOR2_TRANSPARENT,
							  sbaritem->GRADIENT, sbaritem->CORNER, sbaritem->BORDERSTYLE, sbaritem->imageItem);
				}
			}
			BitBlt(hdcReal, 0, 0, width, height, pContainer->cachedDC, 0, 0, SRCCOPY);
			EndPaint(hwndDlg, &ps);
			return 0;
		}
		case WM_SETTEXT:
		case WM_SETICON: {
			if (g_framelessSkinmode) {
				DefWindowProc(hwndDlg, msg, wParam, lParam);
				RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOCHILDREN);
				return 0;
			}
			break;
		}
		case WM_NCHITTEST: {
			RECT r;
			POINT pt;
			int k = 0;
			int clip = myGlobals.bClipBorder;

			if (!pContainer)
				break;

			if (!(pContainer->dwFlags & CNT_NOTITLE))
				break;

			GetWindowRect(hwndDlg, &r);
			GetCursorPos(&pt);
			if (pt.y <= r.bottom && pt.y >= r.bottom - clip - 6) {
				if (pt.x > r.left + clip + 10 && pt.x < r.right - clip - 10)
					return HTBOTTOM;
				if (pt.x < r.left + clip + 10)
					return HTBOTTOMLEFT;
				if (pt.x > r.right - clip - 10)
					return HTBOTTOMRIGHT;

			}
			else if (pt.y >= r.top && pt.y <= r.top + 6) {
				if (pt.x > r.left + clip + 10 && pt.x < r.right - clip - 10)
					return HTTOP;
				if (pt.x < r.left + clip + 10)
					return HTTOPLEFT;
				if (pt.x > r.right - clip - 10)
					return HTTOPRIGHT;
			}
			else if (pt.x >= r.left && pt.x <= r.left + clip + 6)
				return HTLEFT;
			else if (pt.x >= r.right - clip - 6 && pt.x <= r.right)
				return HTRIGHT;

			return(DefWindowProc(hwndDlg, WM_NCHITTEST, wParam, lParam));
		}
		case 0xae:						// must be some undocumented message - seems it messes with the title bar...
			if (g_framelessSkinmode)
				return 0;
		default:
			break;
	}
	return CallWindowProc(OldContainerWndProc, hwndDlg, msg, wParam, lParam);
}
/*
 * container window procedure...
 */

static BOOL fHaveTipper = FALSE;

static BOOL CALLBACK DlgProcContainer(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct ContainerWindowData *pContainer = 0;        // pointer to our struct ContainerWindowData
	int iItem = 0;
	TCITEM item;
	HWND  hwndTab;
	BOOL  bSkinned;

	pContainer = (struct ContainerWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
	bSkinned = pContainer ? pContainer->bSkinned : FALSE;
	hwndTab = GetDlgItem(hwndDlg, IDC_MSGTABS);

	switch (msg) {
		case WM_INITDIALOG: {
			DWORD ws;
			HMENU hSysmenu;
			DWORD dwCreateFlags;
			int iMenuItems;
			int i = 0;
			ButtonItem *pbItem;
			HWND  hwndButton = 0;
			BOOL isFlat = DBGetContactSettingByte(NULL, SRMSGMOD_T, "tbflat", 0);
			BOOL isThemed = !DBGetContactSettingByte(NULL, SRMSGMOD_T, "nlflat", 0);

			fHaveTipper = ServiceExists("mToolTip/ShowTip");

			if (MyMonitorFromWindow == 0)
				MyMonitorFromWindow = (HMONITOR(WINAPI *)(HWND, DWORD)) GetProcAddress(GetModuleHandleA("USER32"), "MonitorFromWindow");
			if (MyGetMonitorInfoA == 0)
				MyGetMonitorInfoA = (BOOL(WINAPI *)(HMONITOR, LPMONITORINFO)) GetProcAddress(GetModuleHandleA("USER32"), "GetMonitorInfoA");

			if (!menuBarNames_done) {
				int j;

				for (j = 0; j < 5; j++)
					menuBarNames_translated[j] = TranslateTS(menuBarNames[j]);
				menuBarNames_done = TRUE;
			}

			OldContainerWndProc = (WNDPROC)SetWindowLong(hwndDlg, GWL_WNDPROC, (LONG)ContainerWndProc);

			if (myGlobals.m_TabAppearance & TCF_FLAT)
				SetWindowLong(hwndTab, GWL_STYLE, GetWindowLong(hwndTab, GWL_STYLE) | TCS_BUTTONS);
			pContainer = (struct ContainerWindowData *) lParam;
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) pContainer);

			pContainer->hwnd = hwndDlg;
			dwCreateFlags = pContainer->dwFlags;

			pContainer->isCloned = (pContainer->dwFlags & CNT_CREATE_CLONED);

			SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);          // set options...
			pContainer->dwFlags |= dwCreateFlags;
			pContainer->buttonItems = g_ButtonSet.items;
			/*
			 * create the button items
			 */

			pbItem = pContainer->buttonItems;

			while (pbItem) {
				hwndButton = CreateWindowEx(0, _T("TSButtonClass"), _T(""), BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | WS_TABSTOP, 0, 0, 5, 5, hwndDlg, (HMENU)pbItem->uId, g_hInst, NULL);
				SendMessage(hwndButton, BUTTONSETASFLATBTN, 0, isFlat ? 0 : 1);
				SendMessage(hwndButton, BUTTONSETASFLATBTN + 10, 0, isThemed ? 1 : 0);

				SendMessage(hwndButton, BUTTONSETASFLATBTN + 20, 0, (LPARAM)pbItem);

				if (pbItem->dwFlags & BUTTON_ISTOGGLE)
					SendMessage(hwndButton, BUTTONSETASPUSHBTN, 0, 0);

				if (pbItem->szTip[0])
					SendMessage(hwndButton, BUTTONADDTOOLTIP, (WPARAM)pbItem->szTip, 0);

				if (pbItem->dwFlags & BUTTON_ISSIDEBAR) {
					if (pbItem->yOff >= 0) {
						pContainer->sb_NrBottomButtons++;
						pContainer->sb_TopHeight += pbItem->height + 2;
					}
					else if (pbItem->yOff < 0) {
						pContainer->sb_NrBottomButtons++;
						pContainer->sb_BottomHeight += pbItem->height + 2;
					}
					ShowWindow(hwndButton, pContainer->dwFlags & CNT_SIDEBAR ? SW_SHOW : SW_HIDE);
				}

				SendMessage(hwndButton, BUTTONSETASFLATBTN + 12, 0, (LPARAM)pContainer);
				pbItem = pbItem->nextItem;
			}

			pContainer->sb_FirstButton = 0;
			pContainer->bSkinned = g_skinnedContainers;
			SetClassLong(hwndDlg, GCL_STYLE, GetClassLong(hwndDlg, GCL_STYLE) & ~(CS_VREDRAW | CS_HREDRAW));
			SetClassLong(hwndTab, GCL_STYLE, GetClassLong(hwndTab, GCL_STYLE) & ~(CS_VREDRAW | CS_HREDRAW));

			SetClassLong(hwndDlg, GCL_STYLE, GetClassLong(hwndDlg, GCL_STYLE) & ~CS_DROPSHADOW);

			/*
			 * additional system menu items...
			 */

			hSysmenu = GetSystemMenu(hwndDlg, FALSE);
			iMenuItems = GetMenuItemCount(hSysmenu);

			InsertMenu(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_SEPARATOR, 0, _T(""));
			InsertMenu(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_STRING, IDM_STAYONTOP, TranslateT("Stay on Top"));
			if (!g_framelessSkinmode)
				InsertMenu(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_STRING, IDM_NOTITLE, TranslateT("Hide titlebar"));
			InsertMenu(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_SEPARATOR, 0, _T(""));
			InsertMenu(hSysmenu, iMenuItems++ - 2, MF_BYPOSITION | MF_STRING, IDM_MOREOPTIONS, TranslateT("Container options..."));
			SetWindowText(hwndDlg, TranslateT("Message Session..."));
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)myGlobals.g_iconContainer);

			/*
			 * make the tab control the controlling parent window for all message dialogs
			 */

			ws = GetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_EXSTYLE);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_EXSTYLE, ws | WS_EX_CONTROLPARENT);

			ws = GetWindowLong(hwndTab, GWL_STYLE);
			if (myGlobals.m_TabAppearance & TCF_SINGLEROWTABCONTROL) {
				ws &= ~TCS_MULTILINE;
				ws |= TCS_SINGLELINE;
				ws |= TCS_FIXEDWIDTH;
			}
			else {
				ws &= ~TCS_SINGLELINE;
				ws |= TCS_MULTILINE;
				if (ws & TCS_BUTTONS)
					ws |= TCS_FIXEDWIDTH;
			}
			SetWindowLong(hwndTab, GWL_STYLE, ws);
			TabCtrl_SetImageList(GetDlgItem(hwndDlg, IDC_MSGTABS), myGlobals.g_hImageList);
			TabCtrl_SetPadding(GetDlgItem(hwndDlg, IDC_MSGTABS), DBGetContactSettingByte(NULL, SRMSGMOD_T, "x-pad", 3), DBGetContactSettingByte(NULL, SRMSGMOD_T, "y-pad", 3));

			SendMessage(hwndDlg, DM_CONFIGURECONTAINER, 0, 10);

			/*
			 * context menu
			 */
			pContainer->hMenuContext = myGlobals.g_hMenuContext;
			/*
			 * tab tooltips...
			 */
			if (!fHaveTipper || DBGetContactSettingByte(NULL, SRMSGMOD_T, "d_tooltips", 1) == 0) {
				pContainer->hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT,
													 CW_USEDEFAULT, CW_USEDEFAULT, hwndDlg, NULL, g_hInst, (LPVOID) NULL);

				if (pContainer->hwndTip) {
					SetWindowPos(pContainer->hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
					TabCtrl_SetToolTips(GetDlgItem(hwndDlg, IDC_MSGTABS), pContainer->hwndTip);
				}

			}
			else
				pContainer->hwndTip = 0;

			if (pContainer->dwFlags & CNT_CREATE_MINIMIZED) {
 				SetWindowLong(hwndDlg, GWL_STYLE, GetWindowLong(hwndDlg, GWL_STYLE) & ~WS_VISIBLE);
 				SendMessage(hwndDlg, DM_RESTOREWINDOWPOS, RWPF_HIDE, 0);
				GetClientRect(hwndDlg, &pContainer->rcSaved);
				ShowWindow(hwndDlg, SW_SHOWMINNOACTIVE);
			}
			else {
				SendMessage(hwndDlg, DM_RESTOREWINDOWPOS, 0, 0);
				//ShowWindow(hwndDlg, SW_SHOWNORMAL);
			}
			SetTimer(hwndDlg, TIMERID_HEARTBEAT, TIMEOUT_HEARTBEAT, NULL);
			SendMessage(hwndDlg, DM_SETSIDEBARBUTTONS, 0, 0);
			return TRUE;
		}
		case DM_RESTOREWINDOWPOS: {
#if defined (_UNICODE)
			char *szSetting = "CNTW_";
#else
			char *szSetting = "CNT_";
#endif
			char szCName[CONTAINER_NAMELEN + 20];
			/*
			 * retrieve the container window geometry information from the database.
			 */
			if (pContainer->isCloned && pContainer->hContactFrom != 0 && !(pContainer->dwFlags & CNT_GLOBALSIZE)) {
				if (MY_Utils_RestoreWindowPosition(hwndDlg, pContainer->hContactFrom, SRMSGMOD_T, "split", wParam)) {
					if (MY_Utils_RestoreWindowPositionNoMove(hwndDlg, pContainer->hContactFrom, SRMSGMOD_T, "split", wParam))
						if (MY_Utils_RestoreWindowPosition(hwndDlg, NULL, SRMSGMOD_T, "split", wParam))
							if (MY_Utils_RestoreWindowPositionNoMove(hwndDlg, NULL, SRMSGMOD_T, "split", wParam))
								SetWindowPos(hwndDlg, 0, 50, 50, 450, 300, SWP_NOZORDER | SWP_NOACTIVATE);
				}
			}
			else {
				if (pContainer->dwFlags & CNT_GLOBALSIZE) {
					if (MY_Utils_RestoreWindowPosition(hwndDlg, NULL, SRMSGMOD_T, "split", wParam))
						if (MY_Utils_RestoreWindowPositionNoMove(hwndDlg, NULL, SRMSGMOD_T, "split", wParam))
							SetWindowPos(hwndDlg, 0, 50, 50, 450, 300, SWP_NOZORDER | SWP_NOACTIVATE);
				}
				else {
					mir_snprintf(szCName, sizeof(szCName), "%s%d", szSetting, pContainer->iContainerIndex);
					if (MY_Utils_RestoreWindowPosition(hwndDlg, NULL, SRMSGMOD_T, szCName, wParam)) {
						if (MY_Utils_RestoreWindowPositionNoMove(hwndDlg, NULL, SRMSGMOD_T, szCName, wParam))
							if (MY_Utils_RestoreWindowPosition(hwndDlg, NULL, SRMSGMOD_T, "split", wParam))
								if (MY_Utils_RestoreWindowPositionNoMove(hwndDlg, NULL, SRMSGMOD_T, "split", wParam))
									SetWindowPos(hwndDlg, 0, 50, 50, 450, 300, SWP_NOZORDER | SWP_NOACTIVATE);
					}
				}
			}
			break;
		}
		case WM_COMMAND: {
			HANDLE hContact;
			struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);
			DWORD dwOldFlags = pContainer->dwFlags;
			int i = 0;
			ButtonItem *pItem = pContainer->buttonItems;

			if (dat) {
				DWORD dwOldMsgWindowFlags = dat->dwFlags;
				DWORD dwOldEventIsShown = dat->dwFlagsEx;

				if (MsgWindowMenuHandler(pContainer->hwndActive, dat, LOWORD(wParam), MENU_PICMENU) == 1)
					break;
			}
			SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
			if (LOWORD(wParam) == IDC_TBFIRSTUID - 1)
				break;
			else if (LOWORD(wParam) >= IDC_TBFIRSTUID) {                    // skinnable buttons handling
				ButtonItem *item = pContainer->buttonItems;
				WPARAM wwParam = 0;
				LPARAM llParam = 0;
				HANDLE hContact = dat ? dat->hContact : 0;
				int serviceFailure = FALSE;

				while (item) {
					if (item->uId == (DWORD)LOWORD(wParam)) {
						int contactOK = ServiceParamsOK(item, &wwParam, &llParam, hContact);

						if (item->dwFlags & BUTTON_ISSERVICE) {
							if (ServiceExists(item->szService) && contactOK)
								CallService(item->szService, wwParam, llParam);
							else if (contactOK)
								serviceFailure = TRUE;
						}
						else if (item->dwFlags & BUTTON_ISPROTOSERVICE) {
							if (contactOK) {
								char szFinalService[512];

								mir_snprintf(szFinalService, 512, "%s/%s", (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), item->szService);
								if (ServiceExists(szFinalService))
									CallService(szFinalService, wwParam, llParam);
								else
									serviceFailure = TRUE;
							}
						}
						else if (item->dwFlags & BUTTON_ISDBACTION) {
							BYTE *pValue;
							char *szModule = item->szModule;
							char *szSetting = item->szSetting;
							HANDLE finalhContact = 0;

							if (item->dwFlags & BUTTON_ISCONTACTDBACTION || item->dwFlags & BUTTON_DBACTIONONCONTACT) {
								contactOK = ServiceParamsOK(item, &wwParam, &llParam, hContact);
								if (contactOK && item->dwFlags & BUTTON_ISCONTACTDBACTION)
									szModule = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
								finalhContact = hContact;
							}
							else
								contactOK = 1;

							if (contactOK) {
								BOOL fDelete = FALSE;

								if (item->dwFlags & BUTTON_ISTOGGLE) {
									BOOL fChecked = (SendMessage(item->hWnd, BM_GETCHECK, 0, 0) == BST_UNCHECKED);

									pValue = fChecked ? item->bValueRelease : item->bValuePush;
									if (fChecked && pValue[0] == 0)
										fDelete = TRUE;
								}
								else
									pValue = item->bValuePush;

								if (fDelete)
									DBDeleteContactSetting(finalhContact, szModule, szSetting);
								else {
									switch (item->type) {
										case DBVT_BYTE:
											DBWriteContactSettingByte(finalhContact, szModule, szSetting, pValue[0]);
											break;
										case DBVT_WORD:
											DBWriteContactSettingWord(finalhContact, szModule, szSetting, *((WORD *)&pValue[0]));
											break;
										case DBVT_DWORD:
											DBWriteContactSettingDword(finalhContact, szModule, szSetting, *((DWORD *)&pValue[0]));
											break;
										case DBVT_ASCIIZ:
											DBWriteContactSettingString(finalhContact, szModule, szSetting, (char *)pValue);
											break;
									}
								}
							}
							else if (item->dwFlags & BUTTON_ISTOGGLE)
								SendMessage(item->hWnd, BM_SETCHECK, 0, 0);
						}
						if (!contactOK)
							MessageBox(0, _T("The requested action requires a valid contact selection. Please select a contact from the contact list and repeat"), _T("Parameter mismatch"), MB_OK);
						if (serviceFailure) {
							char szError[512];

							mir_snprintf(szError, 512, "The service %s specified by the %s button definition was not found. You may need to install additional plugins", item->szService, item->szName);
							MessageBoxA(0, szError, "Service failure", MB_OK);
						}
						goto buttons_done;
					}
					item = item->nextItem;
				}
			}
			while (pItem) {
				if (LOWORD(wParam) == pItem->uId) {
					if (pItem->pfnAction != NULL)
						pItem->pfnAction(pItem, pContainer->hwndActive, dat, GetDlgItem(hwndDlg, pItem->uId));
				}
				pItem = pItem->nextItem;
			}
buttons_done:
			switch (LOWORD(wParam)) {
				case IDC_TOGGLESIDEBAR: {
					RECT rc;
					LONG dwNewLeft;
					DWORD exStyle = GetWindowLong(hwndDlg, GWL_EXSTYLE);
					BOOL skinnedMode = bSkinned;
					ButtonItem *pItem = pContainer->buttonItems;

					if (pfnIsThemeActive)
						skinnedMode |= (pfnIsThemeActive() ? 1 : 0);

					GetWindowRect(hwndDlg, &rc);
					dwNewLeft = pContainer->dwFlags & CNT_SIDEBAR ? -SIDEBARWIDTH : SIDEBARWIDTH;
					while (pItem) {
						ShowWindow(GetDlgItem(hwndDlg, pItem->uId), SW_HIDE);
						pItem = pItem->nextItem;
					}
					SetWindowLong(hwndDlg, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
					SetWindowLong(hwndDlg, GWL_EXSTYLE, exStyle);
					ShowWindow(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN), SW_HIDE);
					ShowWindow(GetDlgItem(hwndDlg, IDC_SIDEBARUP), SW_HIDE);
					pContainer->preSIZE.cx = pContainer->preSIZE.cy = 0;
					pContainer->oldDCSize.cx = pContainer->oldDCSize.cy = 0;

					SetWindowPos(hwndDlg, 0, rc.left + dwNewLeft, rc.top, (rc.right - rc.left) - dwNewLeft, rc.bottom - rc.top,
								 SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOCOPYBITS | SWP_NOZORDER | SWP_DEFERERASE | SWP_ASYNCWINDOWPOS | (dwNewLeft < 0 && skinnedMode ? SWP_NOREDRAW : 0));
					pItem = pContainer->buttonItems;
					while (pItem) {
						if (pItem->dwFlags & BUTTON_ISSIDEBAR)
							ShowWindow(GetDlgItem(hwndDlg, pItem->uId), pContainer->dwFlags & CNT_SIDEBAR ? SW_SHOW : SW_HIDE);
						else
							ShowWindow(GetDlgItem(hwndDlg, pItem->uId), SW_SHOW);
						pItem = pItem->nextItem;
					}
					ShowWindow(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN), pContainer->dwFlags & CNT_SIDEBAR ? SW_SHOW : SW_HIDE);
					ShowWindow(GetDlgItem(hwndDlg, IDC_SIDEBARUP), pContainer->dwFlags & CNT_SIDEBAR ? SW_SHOW : SW_HIDE);
					PostMessage(hwndDlg, WM_SIZE, 0, 0);
					break;

				}
				case IDC_SIDEBARDOWN:
				case IDC_SIDEBARUP: {
					RECT rc;

					GetClientRect(hwndDlg, &rc);
					if (LOWORD(wParam) == IDC_SIDEBARUP)
						pContainer->sb_FirstButton = pContainer->sb_FirstButton > 0 ? --pContainer->sb_FirstButton : 0;
					else
						pContainer->sb_FirstButton += IsWindowEnabled(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN)) ? 1 : 0;
					DrawSideBar(hwndDlg, pContainer, &rc);
					SendMessage(hwndDlg, WM_SIZE, 0, 0);
					SetFocus(GetDlgItem(pContainer->hwndActive, IDC_MESSAGE));
					break;
				}
				case IDC_CLOSE:
					SendMessage(hwndDlg, WM_SYSCOMMAND, SC_CLOSE, 0);
					break;
				case IDC_MINIMIZE:
					PostMessage(hwndDlg, WM_SYSCOMMAND, SC_MINIMIZE, 0);
					break;
				case IDC_MAXIMIZE:
					SendMessage(hwndDlg, WM_SYSCOMMAND, IsZoomed(hwndDlg) ? SC_RESTORE : SC_MAXIMIZE, 0);
					break;
				case IDOK:
					SendMessage(pContainer->hwndActive, WM_COMMAND, wParam, lParam);      // pass the IDOK command to the active child - fixes the "enter not working
					break;
				case ID_FILE_SAVEMESSAGELOGAS:
					SendMessage(pContainer->hwndActive, DM_SAVEMESSAGELOG, 0, 0);
					break;
				case ID_FILE_CLOSEMESSAGESESSION:
					PostMessage(pContainer->hwndActive, WM_CLOSE, 0, 1);
					break;
				case ID_FILE_CLOSE:
					PostMessage(hwndDlg, WM_CLOSE, 0, 1);
					break;
				case ID_VIEW_SHOWSTATUSBAR:
					ApplyContainerSetting(pContainer, CNT_NOSTATUSBAR, pContainer->dwFlags & CNT_NOSTATUSBAR ? 0 : 1);
					break;
				case ID_VIEW_VERTICALMAXIMIZE:
					ApplyContainerSetting(pContainer, CNT_VERTICALMAX, pContainer->dwFlags & CNT_VERTICALMAX ? 0 : 1);
					break;
				case ID_VIEW_BOTTOMTOOLBAR:
					ApplyContainerSetting(pContainer, CNT_BOTTOMTOOLBAR, pContainer->dwFlags & CNT_BOTTOMTOOLBAR ? 0 : 1);
					WindowList_Broadcast(hMessageWindowList, DM_CONFIGURETOOLBAR, 0, 1);
					return 0;
				case ID_VIEW_SHOWTOOLBAR:
					ApplyContainerSetting(pContainer, CNT_HIDETOOLBAR, pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1);
					WindowList_Broadcast(hMessageWindowList, DM_CONFIGURETOOLBAR, 0, 1);
					return 0;
				case ID_VIEW_SHOWMENUBAR:
					ApplyContainerSetting(pContainer, CNT_NOMENUBAR, pContainer->dwFlags & CNT_NOMENUBAR ? 0 : 1);
					break;
				case ID_VIEW_SHOWTITLEBAR:
					ApplyContainerSetting(pContainer, CNT_NOTITLE, pContainer->dwFlags & CNT_NOTITLE ? 0 : 1);
					break;
				case ID_VIEW_TABSATBOTTOM:
					ApplyContainerSetting(pContainer, CNT_TABSBOTTOM, pContainer->dwFlags & CNT_TABSBOTTOM ? 0 : 1);
					break;
				case ID_TITLEBAR_USESTATICCONTAINERICON:
					ApplyContainerSetting(pContainer, CNT_STATICICON, pContainer->dwFlags & CNT_STATICICON ? 0 : 1);
					break;
				case ID_VIEW_SHOWMULTISENDCONTACTLIST:
					SendMessage(pContainer->hwndActive, WM_COMMAND, MAKEWPARAM(IDC_SENDMENU, ID_SENDMENU_SENDTOMULTIPLEUSERS), 0);
					break;
				case ID_VIEW_STAYONTOP:
					SendMessage(hwndDlg, WM_SYSCOMMAND, IDM_STAYONTOP, 0);
					break;
				case ID_CONTAINER_CONTAINEROPTIONS:
					SendMessage(hwndDlg, WM_SYSCOMMAND, IDM_MOREOPTIONS, 0);
					break;
				case ID_EVENTPOPUPS_DISABLEALLEVENTPOPUPS:
					ApplyContainerSetting(pContainer, (CNT_DONTREPORT | CNT_DONTREPORTUNFOCUSED | CNT_ALWAYSREPORTINACTIVE), 0);
					return 0;
				case ID_EVENTPOPUPS_SHOWPOPUPSIFWINDOWISMINIMIZED:
					ApplyContainerSetting(pContainer, CNT_DONTREPORT, pContainer->dwFlags & CNT_DONTREPORT ? 0 : 1);
					return 0;
				case ID_EVENTPOPUPS_SHOWPOPUPSIFWINDOWISUNFOCUSED:
					ApplyContainerSetting(pContainer, CNT_DONTREPORTUNFOCUSED, pContainer->dwFlags & CNT_DONTREPORTUNFOCUSED ? 0 : 1);
					return 0;
				case ID_EVENTPOPUPS_SHOWPOPUPSFORALLINACTIVESESSIONS:
					ApplyContainerSetting(pContainer, CNT_ALWAYSREPORTINACTIVE, pContainer->dwFlags & CNT_ALWAYSREPORTINACTIVE ? 0 : 1);
					return 0;
				case ID_WINDOWFLASHING_DISABLEFLASHING:
					ApplyContainerSetting(pContainer, CNT_NOFLASH, 1);
					ApplyContainerSetting(pContainer, CNT_FLASHALWAYS, 0);
					return 0;
				case ID_WINDOWFLASHING_FLASHUNTILFOCUSED:
					ApplyContainerSetting(pContainer, CNT_NOFLASH, 0);
					ApplyContainerSetting(pContainer, CNT_FLASHALWAYS, 1);
					return 0;
				case ID_WINDOWFLASHING_USEDEFAULTVALUES:
					ApplyContainerSetting(pContainer, (CNT_NOFLASH | CNT_FLASHALWAYS), 0);
					return 0;
				case ID_OPTIONS_SAVECURRENTWINDOWPOSITIONASDEFAULT: {
					WINDOWPLACEMENT wp = {0};

					wp.length = sizeof(wp);
					if (GetWindowPlacement(hwndDlg, &wp)) {
						DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splitx", wp.rcNormalPosition.left);
						DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splity", wp.rcNormalPosition.top);
						DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splitwidth", wp.rcNormalPosition.right - wp.rcNormalPosition.left);
						DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splitheight", wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
					}
					return 0;
				}
				/*
				 * commands from the message log popup will be routed to the
				 * message log menu handler
				 */
				case ID_MESSAGELOG_EXPORTMESSAGELOGSETTINGS:
				case ID_MESSAGELOG_IMPORTMESSAGELOGSETTINGS:
				case ID_MESSAGELOGSETTINGS_FORTHISCONTACT:
				case ID_MESSAGELOGSETTINGS_GLOBAL: {
					struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);

					if(dat) {
						MsgWindowMenuHandler(pContainer->hwndActive, dat, (int)LOWORD(wParam), MENU_LOGMENU);
						return 0;
					}
					break;
				}
				case ID_HELP_ABOUTTABSRMM:
					CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_ABOUT), 0, DlgProcAbout, 0);
					break;
				case ID_HELP_VIEWRELEASENOTES:
					ViewReleaseNotes();
					break;
			}
			if (pContainer->dwFlags != dwOldFlags)
				SendMessage(hwndDlg, DM_CONFIGURECONTAINER, 0, 0);
			break;
		}
		case WM_ENTERSIZEMOVE: {
			RECT rc;
			SIZE sz;
			GetClientRect(GetDlgItem(hwndDlg, IDC_MSGTABS), &rc);
			sz.cx = rc.right - rc.left;
			sz.cy = rc.bottom - rc.top;
			pContainer->oldSize = sz;
			pContainer->bSizingLoop = TRUE;
			break;
		}
		case WM_EXITSIZEMOVE: {
			RECT rc;

			GetClientRect(GetDlgItem(hwndDlg, IDC_MSGTABS), &rc);
			if (!((rc.right - rc.left) == pContainer->oldSize.cx && (rc.bottom - rc.top) == pContainer->oldSize.cy)) {
				DM_ScrollToBottom(pContainer->hwndActive, 0, 0, 0);
				SendMessage(pContainer->hwndActive, WM_SIZE, 0, 0);
			}
			pContainer->bSizingLoop = FALSE;
			break;
		}
		case WM_GETMINMAXINFO: {
			RECT rc;
			POINT pt;
			MINMAXINFO *mmi = (MINMAXINFO *) lParam;

			mmi->ptMinTrackSize.x = 275;

			GetClientRect(GetDlgItem(hwndDlg, IDC_MSGTABS), &rc);
			pt.y = rc.top;
			TabCtrl_AdjustRect(GetDlgItem(hwndDlg, IDC_MSGTABS), FALSE, &rc);
			if (pContainer->dwFlags & CNT_VERTICALMAX || (GetKeyState(VK_CONTROL) & 0x8000)) {
				RECT rcDesktop = {0};
				BOOL fDesktopValid = FALSE;
				int monitorXOffset = 0;
				WINDOWPLACEMENT wp = {0};

				if (MyMonitorFromWindow) {
					HMONITOR hMonitor = MyMonitorFromWindow(hwndDlg, 2);
					if (hMonitor) {
						MONITORINFO mi = { 0 };
						mi.cbSize = sizeof(mi);
						MyGetMonitorInfoA(hMonitor, &mi);
						rcDesktop = mi.rcWork;
						OffsetRect(&rcDesktop, -mi.rcMonitor.left, -mi.rcMonitor.top);
						monitorXOffset = mi.rcMonitor.left;
						fDesktopValid = TRUE;
					}
				}
				if (!fDesktopValid)
					SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0);

				wp.length = sizeof(wp);
				GetWindowPlacement(hwndDlg, &wp);
				mmi->ptMaxSize.y = rcDesktop.bottom - rcDesktop.top;
				mmi->ptMaxSize.x = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
				mmi->ptMaxPosition.x = wp.rcNormalPosition.left - monitorXOffset;
				mmi->ptMaxPosition.y = 0;
				if (IsIconic(hwndDlg)) {
					mmi->ptMaxPosition.x += rcDesktop.left;
					mmi->ptMaxPosition.y += rcDesktop.top;
				}
				if (myGlobals.m_MathModAvail) {
					if (CallService(MTH_GET_PREVIEW_SHOWN, 0, 0)) {
						RECT rc;
						HWND hwndMath = FindWindowA("TfrmPreview", "Preview");
						GetWindowRect(hwndMath, &rc);
						mmi->ptMaxSize.y -= (rc.bottom - rc.top);
					}
				}
			}
			return 0;
		}
		case WM_MOVE:
			if (myGlobals.m_MathModAvail) {
				TMathWindowInfo mathWndInfo;
				RECT windRect;
				GetWindowRect(hwndDlg, &windRect);
				mathWndInfo.top = windRect.top;
				mathWndInfo.left = windRect.left;
				mathWndInfo.right = windRect.right;
				mathWndInfo.bottom = windRect.bottom;
				CallService(MTH_RESIZE, 0, (LPARAM) &mathWndInfo);
			}
			break;
		case WM_SIZE: {
			RECT rcClient, rcUnadjusted;
			int i = 0;
			TCITEM item = {0};
			POINT pt = {0};
			DWORD sbarWidth;
			BOOL  sizeChanged = FALSE;

			if (IsIconic(hwndDlg)) {
				pContainer->dwFlags |= CNT_DEFERREDSIZEREQUEST;
				break;
			}
			if (pContainer->hwndStatus) {
				struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);
				RECT   rcs;

				SendMessage(pContainer->hwndStatus, WM_USER + 101, 0, (LPARAM)dat);
				GetWindowRect(pContainer->hwndStatus, &rcs);

				pContainer->statusBarHeight = (rcs.bottom - rcs.top) + 1;
				if (pContainer->hwndSlist)
					MoveWindow(pContainer->hwndSlist, bSkinned ? 4 : 2, (rcs.bottom - rcs.top) / 2 - 7, 16, 16, FALSE);
				SendMessage(pContainer->hwndStatus, SB_SETTEXT, (WPARAM)(SBT_OWNERDRAW) | 2, (LPARAM)0);

			}
			else
				pContainer->statusBarHeight = 0;

			GetClientRect(hwndDlg, &rcClient);
			CopyRect(&pContainer->rcSaved, &rcClient);
			rcUnadjusted = rcClient;
			sbarWidth = pContainer->dwFlags & CNT_SIDEBAR ? SIDEBARWIDTH : 0;
			if (lParam) {
				if (pContainer->dwFlags & CNT_TABSBOTTOM)
					MoveWindow(hwndTab, pContainer->tBorder_outer_left + sbarWidth, pContainer->tBorder_outer_top, (rcClient.right - rcClient.left) - (pContainer->tBorder_outer_left + pContainer->tBorder_outer_right + sbarWidth), (rcClient.bottom - rcClient.top) - pContainer->statusBarHeight - (pContainer->tBorder_outer_top + pContainer->tBorder_outer_bottom), FALSE);
				else
					MoveWindow(hwndTab, pContainer->tBorder_outer_left + sbarWidth, pContainer->tBorder_outer_top, (rcClient.right - rcClient.left) - (pContainer->tBorder_outer_left + pContainer->tBorder_outer_right + sbarWidth), (rcClient.bottom - rcClient.top) - pContainer->statusBarHeight - (pContainer->tBorder_outer_top + pContainer->tBorder_outer_bottom), FALSE);
			}
			AdjustTabClientRect(pContainer, &rcClient);

			sizeChanged = (((rcClient.right - rcClient.left) != pContainer->preSIZE.cx) || ((rcClient.bottom - rcClient.top) != pContainer->preSIZE.cy));
			if (sizeChanged) {
				pContainer->preSIZE.cx = rcClient.right - rcClient.left;
				pContainer->preSIZE.cy = rcClient.bottom - rcClient.top;
			}
			/*
			 * we care about all client sessions, but we really resize only the active tab (hwndActive)
			 * we tell inactive tabs to resize theirselves later when they get activated (DM_CHECKSIZE
			 * just queues a resize request)
			 */
			for (i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
				item.mask = TCIF_PARAM;
				TabCtrl_GetItem(hwndTab, i, &item);
				if ((HWND)item.lParam == pContainer->hwndActive) {
					MoveWindow((HWND)item.lParam, rcClient.left, rcClient.top, (rcClient.right - rcClient.left), (rcClient.bottom - rcClient.top), TRUE);
					if (!pContainer->bSizingLoop && sizeChanged)
						DM_ScrollToBottom(pContainer->hwndActive, 0, 0, 1);
				}
				else if (sizeChanged)
					SendMessage((HWND)item.lParam, DM_CHECKSIZE, 0, 0);
			}

			RedrawWindow(hwndTab, NULL, NULL, RDW_INVALIDATE | (pContainer->bSizingLoop ? RDW_ERASE : 0));
			RedrawWindow(hwndDlg, NULL, NULL, (bSkinned ? RDW_FRAME : 0) | RDW_INVALIDATE | (pContainer->bSizingLoop || wParam == SIZE_RESTORED ? RDW_ERASE : 0));

			if (pContainer->hwndStatus)
				InvalidateRect(pContainer->hwndStatus, NULL, FALSE);

			if (pContainer->buttonItems && sizeChanged)
				DrawSideBar(hwndDlg, pContainer, &rcUnadjusted);
			if (myGlobals.m_MathModAvail) {
				TMathWindowInfo mathWndInfo;

				RECT windRect;
				GetWindowRect(hwndDlg, &windRect);
				mathWndInfo.top = windRect.top;
				mathWndInfo.left = windRect.left;
				mathWndInfo.right = windRect.right;
				mathWndInfo.bottom = windRect.bottom;
				CallService(MTH_RESIZE, 0, (LPARAM) &mathWndInfo);
			}
			if ((myGlobals.bClipBorder != 0 || myGlobals.bRoundedCorner) && g_framelessSkinmode) {
				HRGN rgn;
				RECT rcWindow;
				int clip = myGlobals.bClipBorder;

				GetWindowRect(hwndDlg, &rcWindow);


				if (myGlobals.bRoundedCorner)
					rgn = CreateRoundRectRgn(clip, clip, (rcWindow.right - rcWindow.left) - clip + 1,
											 (rcWindow.bottom - rcWindow.top) - clip + 1, myGlobals.bRoundedCorner + clip, myGlobals.bRoundedCorner + clip);
				else
					rgn = CreateRectRgn(clip, clip, (rcWindow.right - rcWindow.left) - clip, (rcWindow.bottom - rcWindow.top) - clip);
				SetWindowRgn(hwndDlg, rgn, TRUE);
			}
			else if (g_framelessSkinmode)
				SetWindowRgn(hwndDlg, NULL, TRUE);

			break;
		}
		case DM_UPDATETITLE: {
			HANDLE hContact = 0;
			TCHAR *szNewTitle = NULL;
			struct MessageWindowData *dat = NULL;

			if (lParam) {               // lParam != 0 means sent by a chat window
				TCHAR szText[512];
				dat = (struct MessageWindowData *)GetWindowLong((HWND)wParam, GWL_USERDATA);
				if(!(pContainer->dwFlags & CNT_TITLE_PRIVATE)){
						GetWindowText((HWND)wParam, szText, SIZEOF(szText));
						szText[SIZEOF(szText)-1] = 0;
 						SetWindowText(hwndDlg, szText);
 						if (dat)
 							SendMessage(hwndDlg, DM_SETICON, (WPARAM) ICON_BIG, (LPARAM)(dat->hTabIcon != dat->hTabStatusIcon ? dat->hTabIcon : dat->hTabStatusIcon));
						break;
				}
			}
			if (wParam == 0) {           // no hContact given - obtain the hContact for the active tab
				if (pContainer->hwndActive && IsWindow(pContainer->hwndActive))
					SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
				else
					break;
				dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);
			}
			else {
				HWND hwnd = WindowList_Find(hMessageWindowList, (HANDLE)wParam);
				if (hwnd == 0) {
					SESSION_INFO *si = SM_FindSessionByHCONTACT((HANDLE)wParam);
					if (si) {
						SendMessage(si->hWnd, GC_UPDATETITLE, 0, 0);
						return 0;
					}
				}
				hContact = (HANDLE)wParam;
				if (hwnd && hContact)
					dat = (struct MessageWindowData *)GetWindowLong(hwnd, GWL_USERDATA);
			}
			if (dat) {
				SendMessage(hwndDlg, DM_SETICON, (WPARAM) ICON_BIG, (LPARAM)(dat->hXStatusIcon ? dat->hXStatusIcon : dat->hTabStatusIcon));
				m_LangPackCP = myGlobals.m_LangPackCP;
				szNewTitle = NewTitle(dat->hContact, pContainer->szTitleFormat, dat->szNickname, dat->szStatus, pContainer->szName, dat->uin, dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->idle, dat->codePage, dat->xStatus, dat->wStatus);
				if (szNewTitle) {
					SetWindowText(hwndDlg, szNewTitle);
					free(szNewTitle);
				}
			}
			return 0;
		}
		case WM_TIMER:
			if (wParam == TIMERID_HEARTBEAT) {
				int i;
				TCITEM item = {0};
				DWORD dwTimeout;
				struct MessageWindowData *dat = 0;

				item.mask = TCIF_PARAM;
				if ((dwTimeout = myGlobals.m_TabAutoClose) > 0) {
					int clients = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_MSGTABS));
					HWND *hwndClients = (HWND *)mir_alloc(sizeof(HWND) * (clients + 1));
					for (i = 0; i < clients; i++) {
						TabCtrl_GetItem(hwndTab, i, &item);
						hwndClients[i] = (HWND)item.lParam;
					}
					for (i = 0; i < clients; i++) {
						if (IsWindow(hwndClients[i])) {
							if ((HWND)hwndClients[i] != pContainer->hwndActive)
								pContainer->bDontSmartClose = TRUE;
							SendMessage((HWND)hwndClients[i], DM_CHECKAUTOCLOSE, (WPARAM)(dwTimeout * 60), 0);
							pContainer->bDontSmartClose = FALSE;
						}
					}
					mir_free(hwndClients);
				}
				dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);
				if (dat && dat->bType == SESSIONTYPE_IM) {
					if ((dat->idle || dat->timezone != -1) && pContainer->hwndActive && IsWindow(pContainer->hwndActive))
						InvalidateRect(GetDlgItem(pContainer->hwndActive, IDC_PANELUIN), NULL, FALSE);
				}
			}
			else if (wParam == TIMERID_HOVER) {
				RECT rcWindow;
				GetWindowRect(hwndDlg, &rcWindow);
			}
			break;
		case WM_SYSCOMMAND:
			switch (wParam) {
				case IDM_STAYONTOP:
					SetWindowPos(hwndDlg, (pContainer->dwFlags & CNT_STICKY) ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
					CheckMenuItem(GetSystemMenu(hwndDlg, FALSE), IDM_STAYONTOP, (pContainer->dwFlags & CNT_STICKY) ? MF_BYCOMMAND | MF_UNCHECKED : MF_BYCOMMAND | MF_CHECKED);
					ApplyContainerSetting(pContainer, CNT_STICKY, pContainer->dwFlags & CNT_STICKY ? 0 : 1);
					break;
				case IDM_NOTITLE: {
					pContainer->oldSize.cx = 0;
					pContainer->oldSize.cy = 0;

					CheckMenuItem(GetSystemMenu(hwndDlg, FALSE), IDM_NOTITLE, (pContainer->dwFlags & CNT_NOTITLE) ? MF_BYCOMMAND | MF_UNCHECKED : MF_BYCOMMAND | MF_CHECKED);
					ApplyContainerSetting(pContainer, CNT_NOTITLE, pContainer->dwFlags & CNT_NOTITLE ? 0 : 1);
					break;
				}
				case IDM_MOREOPTIONS:
					if (IsIconic(pContainer->hwnd))
						SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
					if (pContainer->hWndOptions == 0)
						CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), hwndDlg, DlgProcContainerOptions, (LPARAM)pContainer);
					break;
				case SC_MAXIMIZE:
					pContainer->oldSize.cx = pContainer->oldSize.cy = 0;
					break;
				case SC_RESTORE:
					pContainer->oldSize.cx = pContainer->oldSize.cy = 0;
					if (nen_options.bMinimizeToTray && pContainer->bInTray) {
						if (pContainer->bInTray == 2) {
							MaximiseFromTray(hwndDlg, FALSE, &pContainer->restoreRect);
							PostMessage(hwndDlg, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
						}
						else
							MaximiseFromTray(hwndDlg, nen_options.bAnimated, &pContainer->restoreRect);
						DeleteMenu(myGlobals.g_hMenuTrayUnread, (UINT_PTR)pContainer->iContainerIndex + 1, MF_BYCOMMAND);
						pContainer->bInTray = 0;
						return 0;
					}
					if (bSkinned) {
						ShowWindow(hwndDlg, SW_SHOW);
						return 0;
					}
 					//MAD: to fix rare (windows?) bug...
 					ShowWindow(hwndDlg, SW_RESTORE);
 					//
					break;
				case SC_MINIMIZE:
					if (nen_options.bMinimizeToTray && (nen_options.bTraySupport || (nen_options.floaterMode && !nen_options.bFloaterOnlyMin)) && myGlobals.m_WinVerMajor >= 5) {
						pContainer->bInTray = IsZoomed(hwndDlg) ? 2 : 1;
						GetWindowRect(hwndDlg, &pContainer->restoreRect);
						MinimiseToTray(hwndDlg, nen_options.bAnimated);
						return 0;
					}
					break;
			}
			break;
		case DM_SELECTTAB: {
			switch (wParam) {
					int iItems, iCurrent, iNewTab;
					TCITEM item;

				case DM_SELECT_BY_HWND:
					ActivateTabFromHWND(hwndTab, (HWND) lParam);
					break;
				case DM_SELECT_NEXT:
				case DM_SELECT_PREV:
				case DM_SELECT_BY_INDEX:
					iItems = TabCtrl_GetItemCount(hwndTab);
					iCurrent = TabCtrl_GetCurSel(hwndTab);

					if (iItems == 1)
						break;
					if (wParam == DM_SELECT_PREV)
						iNewTab = iCurrent ? iCurrent - 1 : iItems - 1;     // cycle if current is already the leftmost tab..
					else if (wParam == DM_SELECT_NEXT)
						iNewTab = (iCurrent == (iItems - 1)) ? 0 : iCurrent + 1;
					else if (wParam == DM_SELECT_BY_INDEX) {
						if ((int)lParam > iItems)
							break;
						iNewTab = lParam - 1;
					}

					if (iNewTab != iCurrent) {
						struct TabControlData *tabdat = (struct TabControlData *)GetWindowLong(hwndTab, GWL_USERDATA);
						ZeroMemory((void *)&item, sizeof(item));
						item.mask = TCIF_PARAM;
						if (TabCtrl_GetItem(hwndTab, iNewTab, &item)) {
							TabCtrl_SetCurSel(hwndTab, iNewTab);
							ShowWindow(pContainer->hwndActive, SW_HIDE);
							pContainer->hwndActive = (HWND) item.lParam;
							ShowWindow((HWND)item.lParam, SW_SHOW);
							SetFocus(pContainer->hwndActive);
						}
						if(tabdat && tabdat->m_moderntabs)
                              InvalidateRect(hwndTab, NULL, TRUE);
					}
					break;
			}
			break;
		}
		case WM_NOTIFY: {
			NMHDR* pNMHDR = (NMHDR*) lParam;
			if (pContainer != NULL && pContainer->hwndStatus != 0 && ((LPNMHDR)lParam)->hwndFrom == pContainer->hwndStatus) {
				switch (((LPNMHDR)lParam)->code) {
					case NM_CLICK:
					case NM_RCLICK: {
						unsigned int nParts, nPanel;
						NMMOUSE *nm = (NMMOUSE*)lParam;
						RECT rc;

						nParts = SendMessage(pContainer->hwndStatus, SB_GETPARTS, 0, 0);
						if (nm->dwItemSpec == 0xFFFFFFFE) {
							nPanel = 2;
							SendMessage(pContainer->hwndStatus, SB_GETRECT, nPanel, (LPARAM)&rc);
							if (nm->pt.x > rc.left && nm->pt.x < rc.right)
								goto panel_found;
							else
								return FALSE;
						}
						else
							nPanel = nm->dwItemSpec;
panel_found:
						if (nPanel == 2) {
							struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);
							SendMessage(pContainer->hwndStatus, SB_GETRECT, nPanel, (LPARAM)&rc);
							if (dat)
								SI_CheckStatusIconClick(dat, pContainer->hwndStatus, nm->pt, rc, 2, ((LPNMHDR)lParam)->code);
						}
						else if (((LPNMHDR)lParam)->code == NM_RCLICK) {
							POINT pt;
							HANDLE hContact = 0;
							HMENU hMenu;

							GetCursorPos(&pt);
							SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
							if (hContact) {
								int iSel = 0;
								hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) hContact, 0);
								iSel = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
								if (iSel)
									CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(iSel), MPCF_CONTACTMENU), (LPARAM) hContact);
								DestroyMenu(hMenu);
							}
						}
						return TRUE;
					}
				}
				break;
			}
			switch (pNMHDR->code) {
				case TCN_SELCHANGE: {
					ZeroMemory((void *)&item, sizeof(item));
					iItem = TabCtrl_GetCurSel(hwndTab);
					item.mask = TCIF_PARAM;
					if (TabCtrl_GetItem(hwndTab, iItem, &item)) {
						if ((HWND)item.lParam != pContainer->hwndActive) {
							if (pContainer->hwndActive && IsWindow(pContainer->hwndActive))
								ShowWindow(pContainer->hwndActive, SW_HIDE);
						}
						pContainer->hwndActive = (HWND) item.lParam;
						SendMessage((HWND)item.lParam, DM_SAVESIZE, 0, 1);
						ShowWindow((HWND)item.lParam, SW_SHOW);
						if (!IsIconic(hwndDlg))
							SetFocus(pContainer->hwndActive);
					}
					SendMessage(hwndTab, EM_VALIDATEBOTTOM, 0, 0);
					return 0;
				}
				/*
				 * tooltips
				 */
				case TTN_GETDISPINFO: {
					POINT pt;
					int   iItem;
					TCITEM item;
					NMTTDISPINFO *nmtt = (NMTTDISPINFO *) lParam;
					struct MessageWindowData *cdat = 0;
					TCHAR *contactName = 0;
					TCHAR szTtitle[256];

					if (pContainer->hwndTip == 0)
						break;

					GetCursorPos(&pt);
					if ((iItem = GetTabItemFromMouse(hwndTab, &pt)) == -1)
						break;
					ZeroMemory((void *)&item, sizeof(item));
					item.mask = TCIF_PARAM;
					TabCtrl_GetItem(hwndTab, iItem, &item);
					if (item.lParam) {
						cdat = (struct MessageWindowData *) GetWindowLong((HWND)item.lParam, GWL_USERDATA);
						if (cdat) {
							contactName = cdat->szNickname;
							if (contactName) {
								if (cdat->szProto) {
									nmtt->hinst = NULL;
									if (cdat->idle != 0) {
										time_t diff = time(NULL) - cdat->idle;
										int i_hrs = diff / 3600;
										int i_mins = (diff - i_hrs * 3600) / 60;
										int i_secs = diff % 60;

										mir_sntprintf(szTtitle, safe_sizeof(szTtitle), _T("%s (%s) - Idle: %d:%02d:%02d"), contactName, cdat->szStatus[0] ? cdat->szStatus : _T("(undef)"), i_hrs, i_mins, i_secs);
									}
									else
										mir_sntprintf(szTtitle, safe_sizeof(szTtitle), _T("%s (%s)"), contactName, cdat->szStatus[0] ? cdat->szStatus : _T("(undef)"));
									lstrcpyn(nmtt->szText, szTtitle, 80);
									nmtt->szText[79] = 0;
									nmtt->lpszText = nmtt->szText;
									nmtt->uFlags = TTF_IDISHWND;
								}
							}
						}
					}
					break;
				}
				case NM_RCLICK: {
					HMENU subMenu;
					POINT pt, pt1;
					int iSelection, iItem;
					TCITEM item = {0};
					struct MessageWindowData *dat = 0;

					GetCursorPos(&pt);
					pt1 = pt;
					subMenu = GetSubMenu(pContainer->hMenuContext, 0);

					if ((iItem = GetTabItemFromMouse(hwndTab, &pt)) == -1)
						break;

					item.mask = TCIF_PARAM;
					TabCtrl_GetItem(hwndTab, iItem, &item);
					if (item.lParam && IsWindow((HWND)item.lParam))
						dat = (struct MessageWindowData *)GetWindowLong((HWND)item.lParam, GWL_USERDATA);

					if (dat)
						MsgWindowUpdateMenu((HWND)item.lParam, dat, subMenu, MENU_TABCONTEXT);

					iSelection = TrackPopupMenu(subMenu, TPM_RETURNCMD, pt1.x, pt1.y, 0, hwndDlg, NULL);
					if (iSelection >= IDM_CONTAINERMENU) {
						DBVARIANT dbv = {0};
						char szIndex[10];
#if defined (_UNICODE)
						char *szKey = "TAB_ContainersW";
#else
						char *szKey = "TAB_Containers";
#endif
						mir_snprintf(szIndex, 8, "%d", iSelection - IDM_CONTAINERMENU);
						if (iSelection - IDM_CONTAINERMENU >= 0) {
							if (!DBGetContactSettingTString(NULL, szKey, szIndex, &dbv)) {
								SendMessage((HWND)item.lParam, DM_CONTAINERSELECTED, 0, (LPARAM) dbv.ptszVal);
								DBFreeVariant(&dbv);
							}
						}
						return 1;
					}
					switch (iSelection) {
						case ID_TABMENU_CLOSETAB:
							SendMessage(hwndDlg, DM_CLOSETABATMOUSE, 0, (LPARAM)&pt1);
							break;
						case ID_TABMENU_SAVETABPOSITION:
							DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "tabindex", dat->iTabID * 100);
							break;
						case ID_TABMENU_CLEARSAVEDTABPOSITION:
							DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "tabindex");
							break;
						case ID_TABMENU_LEAVECHATROOM: {
							if (dat && dat->bType == SESSIONTYPE_CHAT) {
								SESSION_INFO *si = (SESSION_INFO *)dat->si;
								if (si && dat->hContact) {
									char* szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) dat->hContact, 0);
									if ( szProto )
										CallProtoService( szProto, PS_LEAVECHAT, (WPARAM)dat->hContact, 0 );
								}
							}
							break;
						}
						case ID_TABMENU_ATTACHTOCONTAINER:
							if ((iItem = GetTabItemFromMouse(hwndTab, &pt1)) == -1)
								break;
							ZeroMemory((void *)&item, sizeof(item));
							item.mask = TCIF_PARAM;
							TabCtrl_GetItem(hwndTab, iItem, &item);
							CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SELECTCONTAINER), hwndDlg, SelectContainerDlgProc, (LPARAM) item.lParam);
							break;
						case ID_TABMENU_CONTAINEROPTIONS: {
							if (pContainer->hWndOptions == 0)
								CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), hwndDlg, DlgProcContainerOptions, (LPARAM) pContainer);
							break;
							case ID_TABMENU_CLOSECONTAINER:
								SendMessage(hwndDlg, WM_CLOSE, 0, 0);
								break;
							}
					}
					InvalidateRect(hwndTab, NULL, FALSE);
					return 1;
				}
			}
			break;
		}
		case WM_LBUTTONDOWN: {
			POINT pt;

			if (pContainer->dwFlags & CNT_NOTITLE) {
				GetCursorPos(&pt);
				return SendMessage(hwndDlg, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(pt.x, pt.y));
			}
			break;
		}
		/*
		 * pass the WM_ACTIVATE msg to the active message dialog child
		 */
		case WM_ACTIVATE:
			if (pContainer == NULL)
				break;

			if (myGlobals.m_MathModAvail && LOWORD(wParam == WA_INACTIVE))
				CallService(MTH_HIDE, 0, 0);

			if (LOWORD(wParam == WA_INACTIVE) && (HWND)lParam != myGlobals.g_hwndHotkeyHandler && GetParent((HWND)lParam) != hwndDlg) {
				if (pContainer->dwFlags & CNT_TRANSPARENCY && pSetLayeredWindowAttributes != NULL && !bSkinned)
					pSetLayeredWindowAttributes(hwndDlg, g_ContainerColorKey, (BYTE)HIWORD(pContainer->dwTransparency), (/* pContainer->bSkinned ? LWA_COLORKEY :  */ 0) | (pContainer->dwFlags & CNT_TRANSPARENCY ? LWA_ALPHA : 0));
			}
			pContainer->hwndSaved = 0;

			if (LOWORD(wParam) != WA_ACTIVE)
				break;
		case WM_MOUSEACTIVATE: {
			TCITEM item;
			int curItem = 0;

			if (pContainer == NULL)
				break;

			FlashContainer(pContainer, 0, 0);
			pContainer->dwFlashingStarted = 0;
			pLastActiveContainer = pContainer;
			if (pContainer->dwFlags & CNT_DEFERREDTABSELECT) {
				NMHDR nmhdr;

				pContainer->dwFlags &= ~CNT_DEFERREDTABSELECT;
				SendMessage(hwndDlg, WM_SYSCOMMAND, SC_RESTORE, 0);
				ZeroMemory((void *)&nmhdr, sizeof(nmhdr));
				nmhdr.code = TCN_SELCHANGE;
				nmhdr.hwndFrom = hwndTab;
				nmhdr.idFrom = IDC_MSGTABS;
				SendMessage(hwndDlg, WM_NOTIFY, 0, (LPARAM) &nmhdr);     // do it via a WM_NOTIFY / TCN_SELCHANGE to simulate user-activation
			}
			if (pContainer->dwFlags & CNT_DEFERREDSIZEREQUEST) {
				pContainer->dwFlags &= ~CNT_DEFERREDSIZEREQUEST;
				SendMessage(hwndDlg, WM_SIZE, 0, 0);
			}

			if (pContainer->dwFlags & CNT_TRANSPARENCY && pSetLayeredWindowAttributes != NULL && !bSkinned) {
				DWORD trans = LOWORD(pContainer->dwTransparency);
				pSetLayeredWindowAttributes(hwndDlg, g_ContainerColorKey, (BYTE)trans, (/* pContainer->bSkinned ? LWA_COLORKEY : */ 0) | (pContainer->dwFlags & CNT_TRANSPARENCY ? LWA_ALPHA : 0));
			}
			if (pContainer->dwFlags & CNT_NEED_UPDATETITLE) {
				HANDLE hContact = 0;
				pContainer->dwFlags &= ~CNT_NEED_UPDATETITLE;
				if (pContainer->hwndActive) {
					SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
					if (hContact)
						SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)hContact, 0);
				}
			}
			ZeroMemory((void *)&item, sizeof(item));
			item.mask = TCIF_PARAM;
			if ((curItem = TabCtrl_GetCurSel(hwndTab)) >= 0)
				TabCtrl_GetItem(hwndTab, curItem, &item);
			if (pContainer->dwFlags & CNT_DEFERREDCONFIGURE && curItem >= 0) {
				pContainer->dwFlags &= ~CNT_DEFERREDCONFIGURE;
				pContainer->hwndActive = (HWND) item.lParam;
				SendMessage(hwndDlg, WM_SYSCOMMAND, SC_RESTORE, 0);
				if (pContainer->hwndActive != 0 && IsWindow(pContainer->hwndActive)) {
					ShowWindow(pContainer->hwndActive, SW_SHOW);
					SetFocus(pContainer->hwndActive);
					SendMessage(pContainer->hwndActive, WM_ACTIVATE, WA_ACTIVE, 0);
					RedrawWindow(pContainer->hwndActive, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
				}
			}
			else if (curItem >= 0)
				SendMessage((HWND) item.lParam, WM_ACTIVATE, WA_ACTIVE, 0);
			if (GetMenu(hwndDlg) != 0)
				DrawMenuBar(hwndDlg);
			break;
		}
		case WM_ENTERMENULOOP: {
			POINT pt;
			int pos;
			HMENU hMenu, submenu;
			struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);

			GetCursorPos(&pt);
			pos = MenuItemFromPoint(hwndDlg, pContainer->hMenu, pt);

			hMenu = pContainer->hMenu;
			CheckMenuItem(hMenu, ID_VIEW_SHOWMENUBAR, MF_BYCOMMAND | pContainer->dwFlags & CNT_NOMENUBAR ? MF_UNCHECKED : MF_CHECKED);
			CheckMenuItem(hMenu, ID_VIEW_SHOWSTATUSBAR, MF_BYCOMMAND | pContainer->dwFlags & CNT_NOSTATUSBAR ? MF_UNCHECKED : MF_CHECKED);
			CheckMenuItem(hMenu, ID_VIEW_SHOWAVATAR, MF_BYCOMMAND | dat->showPic ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_VIEW_SHOWTITLEBAR, MF_BYCOMMAND | pContainer->dwFlags & CNT_NOTITLE ? MF_UNCHECKED : MF_CHECKED);
			EnableMenuItem(hMenu, ID_VIEW_SHOWTITLEBAR, pContainer->bSkinned && g_framelessSkinmode ? MF_GRAYED : MF_ENABLED);
			CheckMenuItem(hMenu, ID_VIEW_TABSATBOTTOM, MF_BYCOMMAND | pContainer->dwFlags & CNT_TABSBOTTOM ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_VIEW_VERTICALMAXIMIZE, MF_BYCOMMAND | pContainer->dwFlags & CNT_VERTICALMAX ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_TITLEBAR_USESTATICCONTAINERICON, MF_BYCOMMAND | pContainer->dwFlags & CNT_STATICICON ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_VIEW_SHOWTOOLBAR, MF_BYCOMMAND | pContainer->dwFlags & CNT_HIDETOOLBAR ? MF_UNCHECKED : MF_CHECKED);
			CheckMenuItem(hMenu, ID_VIEW_BOTTOMTOOLBAR, MF_BYCOMMAND | pContainer->dwFlags & CNT_BOTTOMTOOLBAR ? MF_CHECKED : MF_UNCHECKED);

			CheckMenuItem(hMenu, ID_VIEW_SHOWMULTISENDCONTACTLIST, MF_BYCOMMAND | (dat->sendMode & SMODE_MULTIPLE) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_VIEW_STAYONTOP, MF_BYCOMMAND | pContainer->dwFlags & CNT_STICKY ? MF_CHECKED : MF_UNCHECKED);

			CheckMenuItem(hMenu, ID_EVENTPOPUPS_DISABLEALLEVENTPOPUPS, MF_BYCOMMAND | pContainer->dwFlags & (CNT_DONTREPORT | CNT_DONTREPORTUNFOCUSED | CNT_ALWAYSREPORTINACTIVE) ? MF_UNCHECKED : MF_CHECKED);
			CheckMenuItem(hMenu, ID_EVENTPOPUPS_SHOWPOPUPSIFWINDOWISMINIMIZED, MF_BYCOMMAND | pContainer->dwFlags & CNT_DONTREPORT ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_EVENTPOPUPS_SHOWPOPUPSFORALLINACTIVESESSIONS, MF_BYCOMMAND | pContainer->dwFlags & CNT_ALWAYSREPORTINACTIVE ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_EVENTPOPUPS_SHOWPOPUPSIFWINDOWISUNFOCUSED, MF_BYCOMMAND | pContainer->dwFlags & CNT_DONTREPORTUNFOCUSED ? MF_CHECKED : MF_UNCHECKED);

			CheckMenuItem(hMenu, ID_WINDOWFLASHING_USEDEFAULTVALUES, MF_BYCOMMAND | (pContainer->dwFlags & (CNT_NOFLASH | CNT_FLASHALWAYS)) ? MF_UNCHECKED : MF_CHECKED);
			CheckMenuItem(hMenu, ID_WINDOWFLASHING_DISABLEFLASHING, MF_BYCOMMAND | pContainer->dwFlags & CNT_NOFLASH ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_WINDOWFLASHING_FLASHUNTILFOCUSED, MF_BYCOMMAND | pContainer->dwFlags & CNT_FLASHALWAYS ? MF_CHECKED : MF_UNCHECKED);

			submenu = GetSubMenu(hMenu, 2);
			if (dat && submenu) {
				MsgWindowUpdateMenu(pContainer->hwndActive, dat, submenu, MENU_LOGMENU);
			}
			break;
		}
		case DM_CLOSETABATMOUSE: {
			HWND hwndCurrent;
			POINT *pt = (POINT *)lParam;
			int iItem;
			TCITEM item = {0};

			if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "warnonexit", 0)) {
				if (MessageBox(pContainer->hwnd, TranslateTS(szWarnClose), _T("Miranda"), MB_YESNO | MB_ICONQUESTION) == IDNO)
					break;
			}
			hwndCurrent = pContainer->hwndActive;
			if ((iItem = GetTabItemFromMouse(hwndTab, pt)) == -1)
				break;
			ZeroMemory((void *)&item, sizeof(item));
			item.mask = TCIF_PARAM;
			TabCtrl_GetItem(hwndTab, iItem, &item);
			if (item.lParam) {
				if ((HWND) item.lParam != hwndCurrent) {
					pContainer->bDontSmartClose = TRUE;
					SendMessage((HWND) item.lParam, WM_CLOSE, 0, 1);
					RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE);
					pContainer->bDontSmartClose = FALSE;
				}
				else
					SendMessage((HWND) item.lParam, WM_CLOSE, 0, 1);
			}
			break;
		}
		case WM_PAINT: {
			if (bSkinned) {
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwndDlg, &ps);
				EndPaint(hwndDlg, &ps);
				return 0;
			}
			break;
		}
		case WM_ERASEBKGND: {
			if (bSkinned)
				return TRUE;
			break;
		}
		case DM_OPTIONSAPPLIED: {
			DWORD dwLocalFlags = 0;
			DWORD dwLocalTrans = 0;
			char szCname[40];
			int overridePerContact = 1;
			TCHAR *szTitleFormat = NULL;
			char *szThemeName = NULL;
			DBVARIANT dbv = {0};

#if defined (_UNICODE)
			char *szSetting = "CNTW_";
#else
			char *szSetting = "CNT_";
#endif
			if (pContainer->isCloned && pContainer->hContactFrom != 0) {
				mir_snprintf(szCname, 40, "%s_Flags", szSetting);
				dwLocalFlags = DBGetContactSettingDword(pContainer->hContactFrom, SRMSGMOD_T, szCname, 0xffffffff);
				mir_snprintf(szCname, 40, "%s_Trans", szSetting);
				dwLocalTrans = DBGetContactSettingDword(pContainer->hContactFrom, SRMSGMOD_T, szCname, 0xffffffff);
				mir_snprintf(szCname, 40, "%s_titleformat", szSetting);
				szTitleFormat = MY_DBGetContactSettingString(pContainer->hContactFrom, SRMSGMOD_T, szCname);
				pContainer->szThemeFile[0] = 0;
				mir_snprintf(szCname, 40, "%s_theme", szSetting);
				if (!DBGetContactSettingString(pContainer->hContactFrom, SRMSGMOD_T, szCname, &dbv))
					szThemeName = dbv.pszVal;
				if (dwLocalFlags == 0xffffffff || dwLocalTrans == 0xffffffff)
					overridePerContact = 1;
				else
					overridePerContact = 0;
			}
			if (overridePerContact) {
				mir_snprintf(szCname, 40, "%s%d_Flags", szSetting, pContainer->iContainerIndex);
				dwLocalFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, szCname, 0xffffffff);
				mir_snprintf(szCname, 40, "%s%d_Trans", szSetting, pContainer->iContainerIndex);
				dwLocalTrans = DBGetContactSettingDword(NULL, SRMSGMOD_T, szCname, 0xffffffff);
				if (szTitleFormat == NULL) {
					mir_snprintf(szCname, 40, "%s%d_titleformat", szSetting, pContainer->iContainerIndex);
					szTitleFormat = MY_DBGetContactSettingString(pContainer->hContactFrom, SRMSGMOD_T, szCname);
				}
				if (szThemeName == NULL) {
					mir_snprintf(szCname, 40, "%s%d_theme", szSetting, pContainer->iContainerIndex);
					if (!DBGetContactSettingString(NULL, SRMSGMOD_T, szCname, &dbv))
						szThemeName = dbv.pszVal;
				}
			}
			if (dwLocalFlags == 0xffffffff) {
				pContainer->dwFlags = pContainer->dwPrivateFlags = myGlobals.m_GlobalContainerFlags;
				pContainer->dwPrivateFlags |= (CNT_GLOBALSETTINGS);
			}
			else {
				if (!(dwLocalFlags & CNT_NEWCONTAINERFLAGS))
					dwLocalFlags = CNT_FLAGS_DEFAULT;
				pContainer->dwPrivateFlags = dwLocalFlags;
				pContainer->dwFlags = dwLocalFlags & CNT_GLOBALSETTINGS ? myGlobals.m_GlobalContainerFlags : dwLocalFlags;
			}

			if (dwLocalTrans == 0xffffffff)
				pContainer->dwTransparency = myGlobals.m_GlobalContainerTrans;
			else
				pContainer->dwTransparency = pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS ? myGlobals.m_GlobalContainerTrans : dwLocalTrans;

			if (LOWORD(pContainer->dwTransparency) < 50)
				pContainer->dwTransparency = MAKELONG(50, (WORD)HIWORD(pContainer->dwTransparency));
			if (HIWORD(pContainer->dwTransparency) < 50)
				pContainer->dwTransparency = MAKELONG((WORD)LOWORD(pContainer->dwTransparency), 50);

			if (pContainer->dwFlags & CNT_TITLE_PRIVATE) {
				if (szTitleFormat != NULL)
					_tcsncpy(pContainer->szTitleFormat, szTitleFormat, TITLE_FORMATLEN);
				else
					_tcsncpy(pContainer->szTitleFormat, myGlobals.szDefaultTitleFormat, TITLE_FORMATLEN);
			}
			else
				_tcsncpy(pContainer->szTitleFormat, myGlobals.szDefaultTitleFormat, TITLE_FORMATLEN);

			pContainer->szTitleFormat[TITLE_FORMATLEN - 1] = 0;
			if (szTitleFormat != NULL)
				free(szTitleFormat);

			if (szThemeName != NULL) {
				char szNewPath[MAX_PATH];

				MY_pathToAbsolute(szThemeName, szNewPath);
				mir_snprintf(pContainer->szThemeFile, MAX_PATH, "%s", szNewPath);
				DBFreeVariant(&dbv);
			}
			else
				pContainer->szThemeFile[0] = 0;

			pContainer->ltr_templates = pContainer->rtl_templates = 0;
			pContainer->logFonts = 0;
			pContainer->fontColors = 0;
			pContainer->rtfFonts = 0;
			// container theme
			break;
		}
		case DM_STATUSBARCHANGED: {
			RECT rc;
			SendMessage(hwndDlg, WM_SIZE, 0, 0);
			GetWindowRect(hwndDlg, &rc);
			SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, (rc.bottom - rc.top) + 1, SWP_NOZORDER | SWP_NOACTIVATE);
			SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
			RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
			if (pContainer->hwndStatus != 0 && pContainer->hwndActive != 0)
				PostMessage(pContainer->hwndActive, DM_STATUSBARCHANGED, 0, 0);
			break;
		}
		case DM_CONFIGURECONTAINER: {
			DWORD ws, wsold, ex = 0, exold = 0;
			HMENU hSysmenu = GetSystemMenu(hwndDlg, FALSE);
			HANDLE hContact = 0;
			int i = 0;
			UINT sBarHeight;

			if (!myGlobals.m_SideBarEnabled)
				pContainer->dwFlags &= ~CNT_SIDEBAR;

			ShowWindow(GetDlgItem(hwndDlg, IDC_SIDEBARUP), pContainer->dwFlags & CNT_SIDEBAR ? SW_SHOW : SW_HIDE);
			ShowWindow(GetDlgItem(hwndDlg, IDC_SIDEBARDOWN), pContainer->dwFlags & CNT_SIDEBAR ? SW_SHOW : SW_HIDE);

			SendDlgItemMessage(hwndDlg, IDC_SIDEBARUP, BUTTONSETASFLATBTN + 10, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_SIDEBARUP, BUTTONSETASFLATBTN + 12, 0, (LPARAM)pContainer);
			SendDlgItemMessage(hwndDlg, IDC_SIDEBARUP, BUTTONSETASFLATBTN, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_SIDEBARUP, BM_SETIMAGE, IMAGE_ICON, (LPARAM)myGlobals.g_buttonBarIcons[26]);

			SendDlgItemMessage(hwndDlg, IDC_SIDEBARDOWN, BUTTONSETASFLATBTN + 10, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_SIDEBARDOWN, BUTTONSETASFLATBTN + 12, 0, (LPARAM)pContainer);
			SendDlgItemMessage(hwndDlg, IDC_SIDEBARDOWN, BUTTONSETASFLATBTN, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_SIDEBARDOWN, BM_SETIMAGE, IMAGE_ICON, (LPARAM)myGlobals.g_buttonBarIcons[16]);

			ws = wsold = GetWindowLong(hwndDlg, GWL_STYLE);
			if (!g_framelessSkinmode) {
				ws = (pContainer->dwFlags & CNT_NOTITLE) ?
					 ((IsWindowVisible(hwndDlg) ? WS_VISIBLE : 0) | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_THICKFRAME | (g_framelessSkinmode ? WS_SYSMENU : WS_SYSMENU | WS_SIZEBOX)) :
							 ws | WS_CAPTION | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
			}

			SetWindowLong(hwndDlg, GWL_STYLE, ws);

			pContainer->tBorder = DBGetContactSettingByte(NULL, SRMSGMOD_T, "tborder", 2);
			pContainer->tBorder_outer_left = g_ButtonSet.left + DBGetContactSettingByte(NULL, SRMSGMOD_T, (bSkinned ? "S_tborder_outer_left" : "tborder_outer_left"), 2);
			pContainer->tBorder_outer_right = g_ButtonSet.right + DBGetContactSettingByte(NULL, SRMSGMOD_T, (bSkinned ? "S_tborder_outer_right" : "tborder_outer_right"), 2);
			pContainer->tBorder_outer_top = g_ButtonSet.top + DBGetContactSettingByte(NULL, SRMSGMOD_T, (bSkinned ? "S_tborder_outer_top" : "tborder_outer_top"), 2);
			pContainer->tBorder_outer_bottom = g_ButtonSet.bottom + DBGetContactSettingByte(NULL, SRMSGMOD_T, (bSkinned ? "S_tborder_outer_bottom" : "tborder_outer_bottom"), 2);
			sBarHeight = (UINT)DBGetContactSettingByte(NULL, SRMSGMOD_T, (bSkinned ? "S_sbarheight" : "sbarheight"), 0);

			if (LOBYTE(LOWORD(GetVersion())) >= 5  && pSetLayeredWindowAttributes != NULL) {
				DWORD exold;
				ex = exold = GetWindowLong(hwndDlg, GWL_EXSTYLE);
				ex = (pContainer->dwFlags & CNT_TRANSPARENCY && !g_skinnedContainers) ? ex | WS_EX_LAYERED : ex & ~WS_EX_LAYERED;
				SetWindowLong(hwndDlg, GWL_EXSTYLE, ex);
				if (pContainer->dwFlags & CNT_TRANSPARENCY && !bSkinned) {
					DWORD trans = LOWORD(pContainer->dwTransparency);
					pSetLayeredWindowAttributes(hwndDlg, g_ContainerColorKey, (BYTE)trans, (/* pContainer->bSkinned ? LWA_COLORKEY : */ 0) | (pContainer->dwFlags & CNT_TRANSPARENCY ? LWA_ALPHA : 0));
				}

				if ((exold & WS_EX_LAYERED) != (ex & WS_EX_LAYERED))
					RedrawWindow(hwndDlg, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
			}

			if (!g_framelessSkinmode)
				CheckMenuItem(hSysmenu, IDM_NOTITLE, (pContainer->dwFlags & CNT_NOTITLE) ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hSysmenu, IDM_STAYONTOP, pContainer->dwFlags & CNT_STICKY ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED);
			SetWindowPos(hwndDlg, (pContainer->dwFlags & CNT_STICKY) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOCOPYBITS);
			if (ws != wsold) {
				RECT rc;
				GetWindowRect(hwndDlg, &rc);
				if ((ws & WS_CAPTION) != (wsold & WS_CAPTION)) {
					//SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, (rc.bottom - rc.top) + 1, SWP_NOACTIVATE | SWP_FRAMECHANGED);
					SetWindowPos(hwndDlg,  0, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOCOPYBITS);
					RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW);
					if (pContainer->hwndActive != 0)
						DM_ScrollToBottom(pContainer->hwndActive, 0, 0, 0);
				}
			}
			ws = wsold = GetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_STYLE);
			if (pContainer->dwFlags & CNT_TABSBOTTOM)
				ws |= (TCS_BOTTOM);
			else
				ws &= ~(TCS_BOTTOM);
			if ((ws & (TCS_BOTTOM | TCS_MULTILINE)) != (wsold & (TCS_BOTTOM | TCS_MULTILINE))) {
				SetWindowLong(GetDlgItem(hwndDlg, IDC_MSGTABS), GWL_STYLE, ws);
				RedrawWindow(GetDlgItem(hwndDlg, IDC_MSGTABS), NULL, NULL, RDW_INVALIDATE);
			}

			if (pContainer->dwFlags & CNT_NOSTATUSBAR) {
				if (pContainer->hwndStatus) {
					//SetWindowLong(pContainer->hwndStatus, GWL_WNDPROC, (LONG)OldStatusBarproc);
					if (pContainer->hwndSlist)
						DestroyWindow(pContainer->hwndSlist);
					DestroyWindow(pContainer->hwndStatus);
					pContainer->hwndStatus = 0;
					pContainer->statusBarHeight = 0;
					SendMessage(hwndDlg, DM_STATUSBARCHANGED, 0, 0);
				}
			}
			else if (pContainer->hwndStatus == 0) {
				pContainer->hwndStatus = CreateWindowEx(0, _T("TSStatusBarClass"), NULL, SBT_TOOLTIPS | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwndDlg, NULL, g_hInst, NULL);
				if (nen_options.bFloaterInWin)
					pContainer->hwndSlist = CreateWindowExA(0, "TSButtonClass", "", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 1, 2, 18, 18, pContainer->hwndStatus, (HMENU)1001, g_hInst, NULL);
				else
					pContainer->hwndSlist = 0;

				if (sBarHeight && bSkinned)
					SendMessage(pContainer->hwndStatus, SB_SETMINHEIGHT, sBarHeight, 0);

				SendMessage(pContainer->hwndSlist, BUTTONSETASFLATBTN, 0, 0);
				SendMessage(pContainer->hwndSlist, BUTTONSETASFLATBTN + 10, 0, 0);
				SendMessage(pContainer->hwndSlist, BUTTONSETASFLATBTN + 12, 0, (LPARAM)pContainer);
				SendMessage(pContainer->hwndSlist, BM_SETIMAGE, IMAGE_ICON, (LPARAM)myGlobals.g_buttonBarIcons[16]);
				SendMessage(pContainer->hwndSlist, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Show session list (right click to show quick menu)"), 0);
			}
			if (pContainer->dwFlags & CNT_NOMENUBAR) {
				if (pContainer->hMenu)
					SetMenu(hwndDlg, NULL);
			}
			else {
				if (pContainer->hMenu == 0) {
					int i;
					MENUINFO mi = {0};
					MENUITEMINFO mii = {0};

					mii.cbSize = sizeof(mii);
					mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_DATA;
					mii.fType = MFT_OWNERDRAW;
					mii.dwItemData = 0xf0f0f0f0;
					pContainer->hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENUBAR));
					CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM)pContainer->hMenu, 0);
					for (i = 0; i <= 4; i++) {
						mii.wID = 0xffff5000 + i;
						SetMenuItemInfo(pContainer->hMenu, i, TRUE, &mii);
					}
					if (bSkinned && g_MenuBGBrush && fnSetMenuInfo) {
						mi.cbSize = sizeof(mi);
						mi.hbrBack = g_MenuBGBrush;
						mi.fMask = MIM_BACKGROUND;
						fnSetMenuInfo(pContainer->hMenu, &mi);
					}
				}
				SetMenu(hwndDlg, pContainer->hMenu);
				DrawMenuBar(hwndDlg);
			}
			if (pContainer->hwndActive != 0) {
				hContact = 0;
				SendMessage(pContainer->hwndActive, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
				if (hContact)
					SendMessage(hwndDlg, DM_UPDATETITLE, (WPARAM)hContact, 0);
			}
			SendMessage(hwndDlg, WM_SIZE, 0, 0);
			BroadCastContainer(pContainer, DM_CONFIGURETOOLBAR, 0, 1);
			break;
		}
		case DM_SETSIDEBARBUTTONS: {
			/*
			int i = 0;
			for(i = 0; sbarItems[i].uId != 0; i++) {
			    SendDlgItemMessage(hwndDlg, sbarItems[i].uId, BM_SETIMAGE, IMAGE_ICON, (LPARAM)*(sbarItems[i].hIcon));
			    SendDlgItemMessage(hwndDlg, sbarItems[i].uId, BUTTONADDTOOLTIP, (WPARAM)TranslateTS(sbarItems[i].szTip), 0);
			    if(sbarItems[i].dwFlags & SBI_TOGGLE)
			        SendDlgItemMessage(hwndDlg, sbarItems[i].uId, BUTTONSETASPUSHBTN, 0, 0);
			}*/
			break;
		}
		/*
		 * search the first and most recent unread events in all client tabs...
		 * return all information via a RECENTINFO structure (tab indices,
		 * window handles and timestamps).
		 */
		case DM_QUERYRECENT: {
			int i;
			int iItems = TabCtrl_GetItemCount(hwndTab);
			RECENTINFO *ri = (RECENTINFO *)lParam;
			TCITEM item = {0};

			DWORD dwTimestamp, dwMostRecent = 0;

			ri->iFirstIndex = ri->iMostRecent = -1;
			ri->dwFirst = ri->dwMostRecent = 0;
			ri->hwndFirst = ri->hwndMostRecent = 0;

			for (i = 0; i < iItems; i++) {
				item.mask = TCIF_PARAM;
				TabCtrl_GetItem(hwndTab,  i, &item);
				SendMessage((HWND) item.lParam, DM_QUERYLASTUNREAD, 0, (LPARAM)&dwTimestamp);
				if (dwTimestamp > ri->dwMostRecent) {
					ri->dwMostRecent = dwTimestamp;
					ri->iMostRecent = i;
					ri->hwndMostRecent = (HWND) item.lParam;
					if (ri->iFirstIndex == -1) {
						ri->iFirstIndex = i;
						ri->dwFirst = dwTimestamp;
						ri->hwndFirst = (HWND) item.lParam;
					}
				}
			}
			break;
		}
		/*
		 * search tab with either next or most recent unread message and select it
		 */
		case DM_QUERYPENDING: {
			NMHDR nmhdr;
			RECENTINFO ri;

			SendMessage(hwndDlg, DM_QUERYRECENT, 0, (LPARAM)&ri);
			nmhdr.code = TCN_SELCHANGE;

			if (wParam == DM_QUERY_NEXT && ri.iFirstIndex != -1) {
				TabCtrl_SetCurSel(hwndTab,  ri.iFirstIndex);
				SendMessage(hwndDlg, WM_NOTIFY, 0, (LPARAM) &nmhdr);
			}
			if (wParam == DM_QUERY_MOSTRECENT && ri.iMostRecent != -1) {
				TabCtrl_SetCurSel(hwndTab, ri.iMostRecent);
				SendMessage(hwndDlg, WM_NOTIFY, 0, (LPARAM) &nmhdr);
			}
			break;
		}
		/*
		 * msg dialog children are required to report their minimum height requirements
		 */
		case DM_REPORTMINHEIGHT:
			pContainer->uChildMinHeight = ((UINT) lParam > pContainer->uChildMinHeight) ? (UINT) lParam : pContainer->uChildMinHeight;
			return 0;
		case WM_LBUTTONDBLCLK:
			if (!g_framelessSkinmode)
				SendMessage(hwndDlg, WM_SYSCOMMAND, IDM_NOTITLE, 0);
			break;
		case WM_CONTEXTMENU: {
			break;
		}
		case DM_SETICON: {
			HICON hIconMsg = myGlobals.g_IconMsgEvent;
			if ((HICON)lParam == myGlobals.g_buttonBarIcons[5]) {              // always set typing icon, but don't save it...
				SendMessage(hwndDlg, WM_SETICON, wParam, lParam);
				break;
			}

			if ((HICON)lParam != hIconMsg && pContainer->dwFlags & CNT_STATICICON && myGlobals.g_iconContainer != 0)
				lParam = (LPARAM)myGlobals.g_iconContainer;

			if (pContainer->hIcon == STICK_ICON_MSG && (HICON)lParam != hIconMsg && pContainer->dwFlags & CNT_NEED_UPDATETITLE)
				lParam = (LPARAM)hIconMsg;
			//break;          // don't overwrite the new message indicator flag
			SendMessage(hwndDlg, WM_SETICON, wParam, lParam);
			pContainer->hIcon = (lParam == (LPARAM)hIconMsg) ? STICK_ICON_MSG : 0;
			break;
		}
		case WM_DRAWITEM: {
			int cx = myGlobals.m_smcxicon;
			int cy = myGlobals.m_smcyicon;
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
			int id = LOWORD(dis->itemID);

			if (dis->hwndItem == pContainer->hwndStatus) {
				struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);
				if (dat)
					DrawStatusIcons(dat, dis->hDC, dis->rcItem, 2);
				return TRUE;
			}
			if (dis->CtlType == ODT_MENU && (HIWORD(dis->itemID) == 0xffff || dis->itemData == 0xf0f0f0f0) && id >= 0x5000 && id <= 0x5005) {
				SIZE sz;
				HFONT hOldFont;
				TCHAR *menuName = TranslateTS(menuBarNames_translated[id - 0x5000]);
				COLORREF clrBack;

				hOldFont = SelectObject(dis->hDC, GetStockObject(DEFAULT_GUI_FONT));
				if (bSkinned && g_framelessSkinmode)
					OffsetRect(&dis->rcItem, myGlobals.g_realSkinnedFrame_left, myGlobals.g_realSkinnedFrame_caption);
				GetTextExtentPoint32(dis->hDC, menuName, lstrlen(menuName), &sz);
				SetBkMode(dis->hDC, TRANSPARENT);

				if (myGlobals.m_bIsXP) {
					if (pfnIsThemeActive && pfnIsThemeActive())
						clrBack = COLOR_MENUBAR;
					else
						clrBack = COLOR_3DFACE;
				}
				else
					clrBack = COLOR_3DFACE;

				FillRect(dis->hDC, &dis->rcItem, bSkinned && g_MenuBGBrush ? g_MenuBGBrush : GetSysColorBrush(clrBack));

				if (dis->itemState & ODS_SELECTED && !(dis->itemState & ODS_DISABLED)) {
					if (bSkinned) {
						POINT pt;
						HPEN  hPenOld;

						MoveToEx(dis->hDC, dis->rcItem.left, dis->rcItem.bottom - 1, &pt);
						hPenOld = SelectObject(dis->hDC, myGlobals.g_SkinDarkShadowPen);
						LineTo(dis->hDC, dis->rcItem.left, dis->rcItem.top);
						LineTo(dis->hDC, dis->rcItem.right, dis->rcItem.top);
						SelectObject(dis->hDC, myGlobals.g_SkinLightShadowPen);
						MoveToEx(dis->hDC, dis->rcItem.right - 1, dis->rcItem.top + 1, &pt);
						LineTo(dis->hDC, dis->rcItem.right - 1, dis->rcItem.bottom - 1);
						LineTo(dis->hDC, dis->rcItem.left, dis->rcItem.bottom - 1);
						SelectObject(dis->hDC, hPenOld);
					}
					else
						DrawEdge(dis->hDC, &dis->rcItem, BDR_SUNKENINNER, BF_RECT);
				}
				SetTextColor(dis->hDC, !(dis->itemState & ODS_DISABLED) ? (bSkinned ? myGlobals.skinDefaultFontColor : GetSysColor(COLOR_MENUTEXT)) : GetSysColor(COLOR_GRAYTEXT)); ;
				DrawText(dis->hDC, menuName, lstrlen(menuName), &dis->rcItem,
						 DT_VCENTER | DT_SINGLELINE | DT_CENTER);
				SelectObject(dis->hDC, hOldFont);
				return TRUE;
			}
			return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
		}
		case WM_MEASUREITEM: {
			LPMEASUREITEMSTRUCT lpmi = (LPMEASUREITEMSTRUCT)lParam;
			int id = LOWORD(lpmi->itemID);
			if (lpmi->CtlType == ODT_MENU && (HIWORD(lpmi->itemID) == 0xffff || lpmi->itemData == 0xf0f0f0f0) &&  id >= 0x5000 && id <= 0x5005) {
				SIZE sz;
				HFONT hOldFont;
				HDC hdc = GetWindowDC(hwndDlg);
				TCHAR *menuName = TranslateTS(menuBarNames_translated[id - 0x5000]);

				hOldFont = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
				GetTextExtentPoint32(hdc, menuName, lstrlen(menuName), &sz);
				lpmi->itemWidth = sz.cx;
				SelectObject(hdc, hOldFont);
				ReleaseDC(hwndDlg, hdc);
				return TRUE;
			}
			return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
		}
		case DM_QUERYCLIENTAREA: {
			RECT *rc = (RECT *)lParam;
			if (!IsIconic(hwndDlg))
				GetClientRect(hwndDlg, rc);
			else
				CopyRect(rc, &pContainer->rcSaved);
			AdjustTabClientRect(pContainer, rc);
			return 0;
		}
		case DM_SESSIONLIST: {
			SendMessage(myGlobals.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 101, WM_LBUTTONUP);
			break;
		}
		case WM_DESTROY: {
			int i = 0;
			TCITEM item;
			SESSION_INFO *node = m_WndList;

			if (myGlobals.g_FlashAvatarAvail) { // destroy own flash avatar
				FLASHAVATAR fa = {0};
				struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(pContainer->hwndActive, GWL_USERDATA);

				fa.id = 25367;
				fa.cProto = dat ? dat->szProto : NULL;
				CallService(MS_FAVATAR_DESTROY, (WPARAM)&fa, 0);
			}
			ZeroMemory((void *)&item, sizeof(item));
			for (i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
				item.mask = TCIF_PARAM;
				TabCtrl_GetItem(hwndTab, i, &item);
				if (IsWindow((HWND)item.lParam))
					{
					//mad

					struct MessageWindowData *mwdat = (struct MessageWindowData *)GetWindowLong((HWND)item.lParam, GWL_USERDATA);
					if (mwdat->bActualHistory)
						DBWriteContactSettingDword(mwdat->hContact, SRMSGMOD_T, "messagecount", g_sessionshutdown?0:(DWORD)mwdat->messageCount);
					//
					DestroyWindow((HWND) item.lParam);
					}
			}
			KillTimer(hwndDlg, TIMERID_HEARTBEAT);
			pContainer->hwnd = 0;
			pContainer->hwndActive = 0;
			pContainer->hMenuContext = 0;
			SetMenu(hwndDlg, NULL);
			if (pContainer->hwndStatus) {
				DestroyWindow(pContainer->hwndSlist);
				DestroyWindow(pContainer->hwndStatus);
			}

			// free private theme...
			if (pContainer->ltr_templates)
				free(pContainer->ltr_templates);
			if (pContainer->rtl_templates)
				free(pContainer->rtl_templates);
			if (pContainer->logFonts)
				free(pContainer->logFonts);
			if (pContainer->fontColors)
				free(pContainer->fontColors);
			if (pContainer->rtfFonts)
				free(pContainer->rtfFonts);

			if (pContainer->hMenu)
				DestroyMenu(pContainer->hMenu);
			if (pContainer->hwndTip);
			DestroyWindow(pContainer->hwndTip);
			RemoveContainerFromList(pContainer);
			if (myGlobals.m_MathModAvail)
				CallService(MTH_HIDE, 0, 0);
			while (node) {
				if (node->pContainer == pContainer) {
					node->pContainer = 0;
				}
				node = node->next;
			}
			if (pContainer->cachedDC) {
				SelectObject(pContainer->cachedDC, pContainer->oldHBM);
				DeleteObject(pContainer->cachedHBM);
				DeleteDC(pContainer->cachedDC);
			}
			SetWindowLong(hwndDlg, GWL_WNDPROC, (LONG)OldContainerWndProc);
			return 0;
		}
		case WM_NCDESTROY:
			if (pContainer)
				free(pContainer);
			SetWindowLong(hwndDlg, GWL_USERDATA, 0);
			break;
		case WM_CLOSE: {
			//mad
			if (myGlobals.m_HideOnClose&&!lParam)
				ShowWindow(hwndDlg,0);
			else{
			//

			WINDOWPLACEMENT wp;
			char szCName[40];
#if defined (_UNICODE)
			char *szSetting = "CNTW_";
#else
			char *szSetting = "CNT_";
#endif


			if (lParam == 0 && TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_MSGTABS)) > 0) {    // dont ask if container is empty (no tabs)
				int    clients = TabCtrl_GetItemCount(hwndTab), i;
				TCITEM item = {0};
				int    iOpenJobs = 0;

				if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "warnonexit", 0)) {
					if (MessageBox(hwndDlg, TranslateTS(szWarnClose), _T("Miranda"), MB_YESNO | MB_ICONQUESTION) == IDNO)
						return TRUE;
				}
				item.mask = TCIF_PARAM;
				for (i = 0; i < clients; i++) {
					TabCtrl_GetItem(hwndTab, i, &item);
					if (item.lParam && IsWindow((HWND)item.lParam)) {
						SendMessage((HWND)item.lParam, DM_CHECKQUEUEFORCLOSE, 0, (LPARAM)&iOpenJobs);
					}
				}
				if (iOpenJobs && pContainer) {
					LRESULT result;

					if (pContainer->exFlags & CNT_EX_CLOSEWARN)
						return TRUE;

					pContainer->exFlags |= CNT_EX_CLOSEWARN;
					result = WarnPendingJobs(iOpenJobs);
					pContainer->exFlags &= ~CNT_EX_CLOSEWARN;
					if (result == IDNO)
						return TRUE;
				}
			}

			ZeroMemory((void *)&wp, sizeof(wp));
			wp.length = sizeof(wp);
			/*
			* save geometry information to the database...
			*/
			if (!(pContainer->dwFlags & CNT_GLOBALSIZE)) {
				if (GetWindowPlacement(hwndDlg, &wp) != 0) {
					if (pContainer->isCloned && pContainer->hContactFrom != 0) {
						HANDLE hContact;
						int i;
						TCITEM item = {0};

						item.mask = TCIF_PARAM;
						TabCtrl_GetItem(hwndTab, TabCtrl_GetCurSel(hwndTab), &item);
						SendMessage((HWND)item.lParam, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
						DBWriteContactSettingByte(hContact, SRMSGMOD_T, "splitmax", (BYTE)((wp.showCmd==SW_SHOWMAXIMIZED)?1:0));

						for (i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
							if (TabCtrl_GetItem(hwndTab, i, &item)) {
								SendMessage((HWND)item.lParam, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
								DBWriteContactSettingDword(hContact, SRMSGMOD_T, "splitx", wp.rcNormalPosition.left);
								DBWriteContactSettingDword(hContact, SRMSGMOD_T, "splity", wp.rcNormalPosition.top);
								DBWriteContactSettingDword(hContact, SRMSGMOD_T, "splitwidth", wp.rcNormalPosition.right - wp.rcNormalPosition.left);
								DBWriteContactSettingDword(hContact, SRMSGMOD_T, "splitheight", wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
								}
						}
					}
					else {
						_snprintf(szCName, 40, "%s%dx", szSetting, pContainer->iContainerIndex);
						DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, wp.rcNormalPosition.left);
						_snprintf(szCName, 40, "%s%dy", szSetting, pContainer->iContainerIndex);
						DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, wp.rcNormalPosition.top);
						_snprintf(szCName, 40, "%s%dwidth", szSetting, pContainer->iContainerIndex);
						DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, wp.rcNormalPosition.right - wp.rcNormalPosition.left);
						_snprintf(szCName, 40, "%s%dheight", szSetting, pContainer->iContainerIndex);
						DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);

						DBWriteContactSettingByte(NULL, SRMSGMOD_T, "splitmax", (BYTE)((wp.showCmd==SW_SHOWMAXIMIZED)?1:0));
					}
				}
				else
					MessageBoxA(0, "GetWindowPlacement() failed", "Error", MB_OK);
			}
			// clear temp flags which should NEVER be saved...

			if (pContainer->isCloned && pContainer->hContactFrom != 0) {
				HANDLE hContact;
				int i;
				TCITEM item = {0};

				item.mask = TCIF_PARAM;
				pContainer->dwFlags &= ~(CNT_DEFERREDCONFIGURE | CNT_CREATE_MINIMIZED | CNT_DEFERREDSIZEREQUEST | CNT_CREATE_CLONED);
				for (i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
					if (TabCtrl_GetItem(hwndTab, i, &item)) {
						SendMessage((HWND)item.lParam, DM_QUERYHCONTACT, 0, (LPARAM)&hContact);
						_snprintf(szCName, 40, "%s_Flags", szSetting);
						DBWriteContactSettingDword(hContact, SRMSGMOD_T, szCName, pContainer->dwPrivateFlags);
						_snprintf(szCName, 40, "%s_Trans", szSetting);
						DBWriteContactSettingDword(hContact, SRMSGMOD_T, szCName, pContainer->dwTransparency);

						mir_snprintf(szCName, 40, "%s_theme", szSetting);
						if (lstrlenA(pContainer->szThemeFile) > 1) {
							char szNewPath[MAX_PATH];

							MY_pathToRelative(pContainer->szThemeFile, szNewPath);
							DBWriteContactSettingString(hContact, SRMSGMOD_T, szCName, szNewPath);
						}
						else
							DBDeleteContactSetting(hContact, SRMSGMOD_T, szCName);

						if (pContainer->dwFlags & CNT_TITLE_PRIVATE) {
							char *szTitleFormat = NULL;
							mir_snprintf(szCName, 40, "%s_titleformat", szSetting);
#if defined(_UNICODE)
							szTitleFormat = Utf8_Encode(pContainer->szTitleFormat);
							DBWriteContactSettingString(hContact, SRMSGMOD_T, szCName, szTitleFormat);
							free(szTitleFormat);
#else
							szTitleFormat = pContainer->szTitleFormat;
							DBWriteContactSettingString(hContact, SRMSGMOD_T, szCName, szTitleFormat);
#endif
						}
					}
				}
			}
			else {
				pContainer->dwFlags &= ~(CNT_DEFERREDCONFIGURE | CNT_CREATE_MINIMIZED | CNT_DEFERREDSIZEREQUEST | CNT_CREATE_CLONED);
				_snprintf(szCName, 40, "%s%d_Flags", szSetting, pContainer->iContainerIndex);
				DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, pContainer->dwPrivateFlags);
				_snprintf(szCName, 40, "%s%d_Trans", szSetting, pContainer->iContainerIndex);
				DBWriteContactSettingDword(NULL, SRMSGMOD_T, szCName, pContainer->dwTransparency);
				if (pContainer->dwFlags & CNT_TITLE_PRIVATE) {
					char *szTitleFormat = NULL;
					mir_snprintf(szCName, 40, "%s%d_titleformat", szSetting, pContainer->iContainerIndex);
#if defined(_UNICODE)
					szTitleFormat = Utf8_Encode(pContainer->szTitleFormat);
					DBWriteContactSettingString(NULL, SRMSGMOD_T, szCName, szTitleFormat);
					free(szTitleFormat);
#else
					szTitleFormat = pContainer->szTitleFormat;
					DBWriteContactSettingString(NULL, SRMSGMOD_T, szCName, szTitleFormat);
#endif
				}
				mir_snprintf(szCName, 40, "%s%d_theme", szSetting, pContainer->iContainerIndex);
				if (lstrlenA(pContainer->szThemeFile) > 1) {
					char szNewPath[MAX_PATH];

					MY_pathToRelative(pContainer->szThemeFile, szNewPath);
					DBWriteContactSettingString(NULL, SRMSGMOD_T, szCName, szNewPath);
				}
				else
					DBDeleteContactSetting(NULL, SRMSGMOD_T, szCName);
			}
			DestroyWindow(hwndDlg);
		}//mad_
			break;
		}
		default:
			return FALSE;
	}
	return FALSE;
}

/*
 * search the list of tabs and return the tab (by index) which "belongs" to the given
 * hwnd. The hwnd is the handle of a message dialog childwindow. At creation,
 * the dialog handle is stored in the TCITEM.lParam field, because we need
 * to know the owner of the tab.
 *
 * hwndTab: handle of the tab control itself.
 * hwnd: handle of a message dialog.
 *
 * returns the tab index (zero based), -1 if no tab is found (which SHOULD not
 * really happen, but who knows... ;) )
 */

int GetTabIndexFromHWND(HWND hwndTab, HWND hwnd)
{
	TCITEM item;
	int i = 0;
	int iItems;

	iItems = TabCtrl_GetItemCount(hwndTab);

	ZeroMemory((void *)&item, sizeof(item));
	item.mask = TCIF_PARAM;

	for (i = 0; i < iItems; i++) {
		TabCtrl_GetItem(hwndTab, i, &item);
		if ((HWND)item.lParam == hwnd) {
			return i;
		}
	}
	return -1;
}

/*
 * activates the tab belonging to the given client HWND (handle of the actual
 * message window.
 */

int ActivateTabFromHWND(HWND hwndTab, HWND hwnd)
{
	NMHDR nmhdr;

	int iItem = GetTabIndexFromHWND(hwndTab, hwnd);
	if (iItem >= 0) {
		TabCtrl_SetCurSel(hwndTab, iItem);
		ZeroMemory((void *)&nmhdr, sizeof(nmhdr));
		nmhdr.code = TCN_SELCHANGE;
		SendMessage(GetParent(hwndTab), WM_NOTIFY, 0, (LPARAM) &nmhdr);     // do it via a WM_NOTIFY / TCN_SELCHANGE to simulate user-activation
		return iItem;
	}
	return -1;
}

/*
 * returns the index of the tab under the mouse pointer. Used for
 * context menu popup and tooltips
 * pt: mouse coordinates, obtained from GetCursorPos()
 */

int GetTabItemFromMouse(HWND hwndTab, POINT *pt)
{
	TCHITTESTINFO tch;

	ScreenToClient(hwndTab, pt);
	tch.pt = *pt;
	tch.flags = 0;
	return TabCtrl_HitTest(hwndTab, &tch);
}

/*
 * cut off contact name to the option value set via Options->Tabbed messaging
 * some people were requesting this, because really long contact list names
 * are causing extraordinary wide tabs and these are looking ugly and wasting
 * screen space.
 *
 * size = max length of target string
 */

int CutContactName(TCHAR *oldname, TCHAR *newname, unsigned int size)
{
	int cutMax = myGlobals.m_CutContactNameTo;

	if ((int)lstrlen(oldname) <= cutMax) {
		lstrcpyn(newname, oldname, size);
		newname[size - 1] = 0;
	}
	else {
		TCHAR fmt[20];
		_sntprintf(fmt, 18, _T("%%%d.%ds..."), cutMax, cutMax);
		_sntprintf(newname, size, fmt, oldname);
		newname[size - 1] = 0;
	}
	return 0;
}

/*
 * functions for handling the linked list of struct ContainerWindowData *foo
 */

static struct ContainerWindowData *AppendToContainerList(struct ContainerWindowData *pContainer) {
	struct ContainerWindowData *pCurrent = 0;

	if (!pFirstContainer) {
		pFirstContainer = pContainer;
		pFirstContainer->pNextContainer = NULL;
		return pFirstContainer;
	} else {
		pCurrent = pFirstContainer;
		while (pCurrent->pNextContainer != 0)
			pCurrent = pCurrent->pNextContainer;
		pCurrent->pNextContainer = pContainer;
		pContainer->pNextContainer = NULL;
		return pCurrent;
	}
}

struct ContainerWindowData *FindContainerByName(const TCHAR *name) {
	struct ContainerWindowData *pCurrent = pFirstContainer;

	if (name == NULL || _tcslen(name) == 0)
		return 0;

	if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0)) {            // single window mode - always return 0 and force a new container
		return NULL;
	}

	while (pCurrent) {
		if (!_tcsncmp(pCurrent->szName, name, CONTAINER_NAMELEN))
			return pCurrent;
		pCurrent = pCurrent->pNextContainer;
	}
	// error, didn't find it.
	return NULL;
}

static struct ContainerWindowData *RemoveContainerFromList(struct ContainerWindowData *pContainer) {
	struct ContainerWindowData *pCurrent = pFirstContainer;

	if (pContainer == pFirstContainer) {
		if (pContainer->pNextContainer != NULL)
			pFirstContainer = pContainer->pNextContainer;
		else
			pFirstContainer = NULL;

		if (pLastActiveContainer == pContainer)     // make sure, we don't reference this container anymore
			pLastActiveContainer = pFirstContainer;

		return pFirstContainer;
	}

	do {
		if (pCurrent->pNextContainer == pContainer) {
			pCurrent->pNextContainer = pCurrent->pNextContainer->pNextContainer;

			if (pLastActiveContainer == pContainer)     // make sure, we don't reference this container anymore
				pLastActiveContainer = pFirstContainer;

			return 0;
		}
	} while (pCurrent = pCurrent->pNextContainer);
	return NULL;
}

/*
 * calls the TabCtrl_AdjustRect to calculate the "real" client area of the tab.
 * also checks for the option "hide tabs when only one tab open" and adjusts
 * geometry if necessary
 * rc is the RECT obtained by GetClientRect(hwndTab)
 */

void AdjustTabClientRect(struct ContainerWindowData *pContainer, RECT *rc)
{
	HWND hwndTab = GetDlgItem(pContainer->hwnd, IDC_MSGTABS);
	RECT rcTab, rcTabOrig;
	DWORD dwBottom, dwTop;
	DWORD tBorder = pContainer->tBorder;
	DWORD dwStyle = GetWindowLong(hwndTab, GWL_STYLE);

	GetClientRect(hwndTab, &rcTab);
	dwBottom = rcTab.bottom;
	dwTop = rcTab.top;
	if (pContainer->iChilds > 1 || !(pContainer->dwFlags & CNT_HIDETABS)) {
		DWORD dwTopPad;
		rcTabOrig = rcTab;
		TabCtrl_AdjustRect(hwndTab, FALSE, &rcTab);
		dwTopPad = rcTab.top - rcTabOrig.top;

		rc->left += tBorder;
		rc->right -= tBorder;

		if (dwStyle & TCS_BUTTONS) {
			if (pContainer->dwFlags & CNT_TABSBOTTOM) {
				int nCount = TabCtrl_GetItemCount(hwndTab);
				RECT rcItem;

				if (nCount > 0) {
					TabCtrl_GetItemRect(hwndTab, nCount - 1, &rcItem);
					//rc->top = pContainer->tBorder_outer_top;
					rc->bottom = rcItem.top;
				}
			}
			else {
				rc->top += (dwTopPad - 2);;
				rc->bottom = rcTabOrig.bottom;
			}
		}
		else {
			if (pContainer->dwFlags & CNT_TABSBOTTOM)
				rc->bottom = rcTab.bottom + 2;
			else {
				rc->top += (dwTopPad - 2);;
				rc->bottom = rcTabOrig.bottom;
			}
		}

		rc->top += tBorder;
		rc->bottom -= tBorder;
	}
	else {
		rc->bottom -= pContainer->statusBarHeight;
		rc->bottom -= (pContainer->tBorder_outer_top + pContainer->tBorder_outer_bottom);
	}
	rc->right -= (pContainer->tBorder_outer_left + pContainer->tBorder_outer_right);
	if (pContainer->dwFlags & CNT_SIDEBAR)
		rc->right -= SIDEBARWIDTH;
}

/*
 * retrieve the container name for the given contact handle.
 * if none is assigned, return the name of the default container
 */

int GetContainerNameForContact(HANDLE hContact, TCHAR *szName, int iNameLen)
{
	DBVARIANT dbv;

	if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0)) {           // single window mode using cloned (temporary) containers
		_tcsncpy(szName, _T("Message Session"), iNameLen);
		return 0;
	}

	if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "useclistgroups", 0)) {       // use clist group names for containers...
		if (DBGetContactSettingTString(hContact, "CList", "Group", &dbv)) {
			_tcsncpy(szName, _T("default"), iNameLen);
			return 0;
		}
		else {
			if (lstrlen(dbv.ptszVal) > CONTAINER_NAMELEN)
				dbv.ptszVal[CONTAINER_NAMELEN] = '\0';
			_tcsncpy(szName, dbv.ptszVal, iNameLen);
			szName[iNameLen] = '\0';
			DBFreeVariant(&dbv);
			return dbv.cchVal;
		}
	}
#if defined (_UNICODE)
	if (DBGetContactSettingTString(hContact, SRMSGMOD_T, "containerW", &dbv)) {
#else
	if (DBGetContactSettingTString(hContact, SRMSGMOD_T, "container", &dbv)) {
#endif
		_tcsncpy(szName, _T("default"), iNameLen);
		return 0;
	}
	if (dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_WCHAR) {
		_tcsncpy(szName, dbv.ptszVal, iNameLen);
		szName[iNameLen] = 0;
		DBFreeVariant(&dbv);
		return dbv.cpbVal;
	}
	DBFreeVariant(&dbv);
	return 0;
}

void DeleteContainer(int iIndex) {
	DBVARIANT dbv;
	char szIndex[10], szSetting[CONTAINER_NAMELEN + 30];
#if defined (_UNICODE)
	char *szKey = "TAB_ContainersW";
	char *szSettingP = "CNTW_";
	char *szSubKey = "containerW";
#else
	char *szSettingP = "CNT_";
	char *szKey = "TAB_ContainersW";
	char *szSubKey = "container";
#endif
	HANDLE hhContact;
	_snprintf(szIndex, 8, "%d", iIndex);


	if (!DBGetContactSettingTString(NULL, szKey, szIndex, &dbv)) {
		if (dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_WCHAR) {
			TCHAR *wszContainerName = dbv.ptszVal;
			DBWriteContactSettingTString(NULL, szKey, szIndex, _T("**free**"));

			hhContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
			while (hhContact) {
				DBVARIANT dbv_c;
				if (!DBGetContactSettingTString(hhContact, SRMSGMOD_T, szSubKey, &dbv_c)) {
					TCHAR *wszString = dbv_c.ptszVal;
					if (_tcscmp(wszString, wszContainerName) && lstrlen(wszString) == lstrlen(wszContainerName))
						DBDeleteContactSetting(hhContact, SRMSGMOD_T, "containerW");
					DBFreeVariant(&dbv_c);
				}
				hhContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hhContact, 0);
			}
			_snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%d_Flags", szSettingP, iIndex);
			DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
			_snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%d_Trans", szSettingP, iIndex);
			DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
			_snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%dwidth", szSettingP, iIndex);
			DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
			_snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%dheight", szSettingP, iIndex);
			DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
			_snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%dx", szSettingP, iIndex);
			DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
			_snprintf(szSetting, CONTAINER_NAMELEN + 15, "%s%dy", szSettingP, iIndex);
			DBDeleteContactSetting(NULL, SRMSGMOD_T, szSetting);
		}
		DBFreeVariant(&dbv);
	}
}

void RenameContainer(int iIndex, const TCHAR *szNew) {
	DBVARIANT dbv;
#if defined (_UNICODE)
	char *szKey = "TAB_ContainersW";
	char *szSettingP = "CNTW_";
	char *szSubKey = "containerW";
#else
	char *szSettingP = "CNT_";
	char *szKey = "TAB_ContainersW";
	char *szSubKey = "container";
#endif
	char szIndex[10];
	HANDLE hhContact;

	_snprintf(szIndex, 8, "%d", iIndex);
	if (!DBGetContactSettingTString(NULL, szKey, szIndex, &dbv)) {
		if (szNew != NULL) {
			if (_tcslen(szNew) != 0)
				DBWriteContactSettingTString(NULL, szKey, szIndex, szNew);
		}
		hhContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		while (hhContact) {
			DBVARIANT dbv_c;
			if (!DBGetContactSettingTString(hhContact, SRMSGMOD_T, szSubKey, &dbv_c)) {
				if (!_tcscmp(dbv.ptszVal, dbv_c.ptszVal) && lstrlen(dbv_c.ptszVal) == lstrlen(dbv.ptszVal)) {
					if (szNew != NULL) {
						if (lstrlen(szNew) != 0)
							DBWriteContactSettingTString(hhContact, SRMSGMOD_T, szSubKey, szNew);
					}
				}
				DBFreeVariant(&dbv_c);
			}
			hhContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hhContact, 0);
		}
		DBFreeVariant(&dbv);
	}
}

HMENU BuildContainerMenu()
{
#if defined (_UNICODE)
	char *szKey = "TAB_ContainersW";
#else
	char *szKey = "TAB_Containers";
#endif
	char szCounter[10];
	int i = 0;
	DBVARIANT dbv = { 0 };
	HMENU hMenu;
	MENUITEMINFO mii = {0};

	if (myGlobals.g_hMenuContainer != 0) {
		HMENU submenu = GetSubMenu(myGlobals.g_hMenuContext, 0);
		RemoveMenu(submenu, 6, MF_BYPOSITION);
		DestroyMenu(myGlobals.g_hMenuContainer);
		myGlobals.g_hMenuContainer = 0;
	}

	// no container attach menu, if we are using the "clist group mode"
	if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "useclistgroups", 0) || DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0))
		return NULL;

	hMenu = CreateMenu();
	do {
		_snprintf(szCounter, 8, "%d", i);
		if (DBGetContactSettingTString(NULL, szKey, szCounter, &dbv))
			break;

		if (dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_WCHAR) {
			if (_tcsncmp(dbv.ptszVal, _T("**free**"), CONTAINER_NAMELEN)) {
				AppendMenu(hMenu, MF_STRING, IDM_CONTAINERMENU + i, dbv.ptszVal);
			}
		}
		DBFreeVariant(&dbv);
		i++;
	}
	while (TRUE);

	InsertMenu(myGlobals.g_hMenuContext, ID_TABMENU_ATTACHTOCONTAINER, MF_BYCOMMAND | MF_POPUP, (UINT_PTR) hMenu, TranslateT("Attach to"));
	myGlobals.g_hMenuContainer = hMenu;
	return hMenu;
}

HMENU BuildMCProtocolMenu(HWND hwndDlg) {
	HMENU hMCContextMenu = 0, hMCSubForce = 0, hMCSubDefault = 0, hMenu = 0;
	DBVARIANT dbv;
	int iNumProtos = 0, i = 0, iDefaultProtoByNum = 0;
	char  szTemp[50], *szProtoMostOnline = NULL;
	TCHAR szMenuLine[128], *nick = NULL, *szStatusText = NULL, *tzProtoName = NULL;
	HANDLE hContactMostOnline, handle;
	int    iChecked, isForced;
	WORD   wStatus;

	struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);
	if (dat == NULL)
		return (HMENU) 0;

	if (!IsMetaContact(hwndDlg, dat))
		return (HMENU) 0;

	hMenu = CreatePopupMenu();
	hMCContextMenu = GetSubMenu(hMenu, 0);
	hMCSubForce = CreatePopupMenu();
	hMCSubDefault = CreatePopupMenu();

	AppendMenu(hMenu, MF_STRING | MF_DISABLED | MF_GRAYED | MF_CHECKED, 1, TranslateT("Meta Contact"));
	AppendMenu(hMenu, MF_SEPARATOR, 1, _T(""));

	iNumProtos = (int)CallService(MS_MC_GETNUMCONTACTS, (WPARAM)dat->hContact, 0);
	iDefaultProtoByNum = (int)CallService(MS_MC_GETDEFAULTCONTACTNUM, (WPARAM)dat->hContact, 0);
	hContactMostOnline = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)dat->hContact, 0);
	szProtoMostOnline = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContactMostOnline, 0);
	isForced = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "tabSRMM_forced", -1);

	for (i = 0; i < iNumProtos; i++) {
		mir_snprintf(szTemp, sizeof(szTemp), "Protocol%d", i);
		if (DBGetContactSettingString(dat->hContact, myGlobals.szMetaName, szTemp, &dbv))
			continue;
#if defined(_UNICODE)
		tzProtoName = (TCHAR *)CallService(MS_LANGPACK_PCHARTOTCHAR, 0, (LPARAM)dbv.pszVal);
#else
		tzProtoName = dbv.pszVal;
#endif
		mir_snprintf(szTemp, sizeof(szTemp), "Handle%d", i);
		if ((handle = (HANDLE)DBGetContactSettingDword(dat->hContact, myGlobals.szMetaName, szTemp, 0)) != 0) {
			nick = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)handle, GCDNF_TCHAR);
			mir_snprintf(szTemp, sizeof(szTemp), "Status%d", i);
			wStatus = (WORD)DBGetContactSettingWord(dat->hContact, myGlobals.szMetaName, szTemp, 0);
			szStatusText = (TCHAR *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, wStatus, GCMDF_TCHAR);
		}
		mir_sntprintf(szMenuLine, safe_sizeof(szMenuLine), _T("%s: %s [%s] %s"), tzProtoName, nick, szStatusText, i == isForced ? TranslateT("(Forced)") : _T(""));
#if defined(_UNICODE)
		if (tzProtoName)
			mir_free(tzProtoName);
#endif
		iChecked = MF_UNCHECKED;
		if (hContactMostOnline != 0 && hContactMostOnline == handle)
			iChecked = MF_CHECKED;
		AppendMenu(hMCSubForce, MF_STRING | iChecked, 100 + i, szMenuLine);
		AppendMenu(hMCSubDefault, MF_STRING | (i == iDefaultProtoByNum ? MF_CHECKED : MF_UNCHECKED), 1000 + i, szMenuLine);
		DBFreeVariant(&dbv);
	}
	AppendMenu(hMCSubForce, MF_SEPARATOR, 900, _T(""));
	AppendMenu(hMCSubForce, MF_STRING | ((isForced == -1) ? MF_CHECKED : MF_UNCHECKED), 999, TranslateT("Autoselect"));
	InsertMenu(hMenu, 2, MF_BYPOSITION | MF_POPUP, (UINT_PTR) hMCSubForce, TranslateT("Use Protocol"));
	InsertMenu(hMenu, 2, MF_BYPOSITION | MF_POPUP, (UINT_PTR) hMCSubDefault, TranslateT("Set Default Protocol"));

	return hMenu;
}

/*
 * flashes the container
 * iMode != 0: turn on flashing
 * iMode == 0: turn off flashing
 */

void FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iCount) {
	FLASHWINFO fwi;

	if (MyFlashWindowEx == NULL)
		return;

	if (pContainer->bInTray && iMode != 0 && nen_options.iAutoRestore > 0) {
		BOOL old = nen_options.bMinimizeToTray;
		nen_options.bMinimizeToTray = FALSE;
		ShowWindow(pContainer->hwnd, SW_HIDE);
		SetParent(pContainer->hwnd, NULL);
		SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		nen_options.bMinimizeToTray = old;
		SetWindowLong(pContainer->hwnd, GWL_STYLE, GetWindowLong(pContainer->hwnd, GWL_STYLE) | WS_VISIBLE);
		pContainer->bInTray = 0;
	}

	if (pContainer->dwFlags & CNT_NOFLASH)                  // container should never flash
		return;

	fwi.cbSize = sizeof(fwi);
	fwi.uCount = 0;

	if (iMode) {
		fwi.dwFlags = FLASHW_ALL;
		if (pContainer->dwFlags & CNT_FLASHALWAYS)
			fwi.dwFlags |= FLASHW_TIMER;
		else
			fwi.uCount = (iCount == 0) ? DBGetContactSettingByte(NULL, SRMSGMOD_T, "nrflash", 4) : iCount;
		fwi.dwTimeout = DBGetContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", 1000);

	}
	else
		fwi.dwFlags = FLASHW_STOP;

	fwi.hwnd = pContainer->hwnd;
	pContainer->dwFlashingStarted = GetTickCount();
	MyFlashWindowEx(&fwi);
}

void ReflashContainer(struct ContainerWindowData *pContainer) {
	DWORD dwStartTime = pContainer->dwFlashingStarted;

	if (GetForegroundWindow() == pContainer->hwnd || GetActiveWindow() == pContainer->hwnd)       // dont care about active windows
		return;

	if (pContainer->dwFlags & CNT_NOFLASH || pContainer->dwFlashingStarted == 0)
		return;                                                                                 // dont care about containers which should never flash

	if (pContainer->dwFlags & CNT_FLASHALWAYS)
		FlashContainer(pContainer, 1, 0);
	else {
		// recalc the remaining flashes
		DWORD dwInterval = DBGetContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", 1000);
		int iFlashesElapsed = (GetTickCount() - dwStartTime) / dwInterval;
		DWORD dwFlashesDesired = DBGetContactSettingByte(NULL, SRMSGMOD_T, "nrflash", 4);
		if (iFlashesElapsed < (int)dwFlashesDesired)
			FlashContainer(pContainer, 1, dwFlashesDesired - iFlashesElapsed);
		else {
			BOOL isFlashed = FlashWindow(pContainer->hwnd, TRUE);
			if (!isFlashed)
				FlashWindow(pContainer->hwnd, TRUE);
		}
	}
	pContainer->dwFlashingStarted = dwStartTime;
}

/*
 * broadcasts a message to all child windows (tabs/sessions)
 */

void BroadCastContainer(struct ContainerWindowData *pContainer, UINT message, WPARAM wParam, LPARAM lParam) {
	int i;
	TCITEM item;

	HWND hwndTab = GetDlgItem(pContainer->hwnd, IDC_MSGTABS);
	ZeroMemory((void *)&item, sizeof(item));

	item.mask = TCIF_PARAM;

	for (i = 0; i < TabCtrl_GetItemCount(hwndTab); i++) {
		TabCtrl_GetItem(hwndTab, i, &item);
		if (IsWindow((HWND)item.lParam))
			SendMessage((HWND)item.lParam, message, wParam, lParam);
	}
}

/*
 * this updates the container menu bar and other window elements depending on the current child
 * session (IM, chat etc.). It fully supports IEView and will disable/enable the message log menus
 * depending on the configuration of IEView (e.g. when template mode is on, the message log settin
 * menus have no functionality, thus can be disabled to improve ui feedback quality).
 */

void UpdateContainerMenu(HWND hwndDlg, struct MessageWindowData *dat) {
	char szTemp[100];
	BYTE IEViewMode = 0;
	BOOL fDisable = FALSE;

	if (dat->bType == SESSIONTYPE_CHAT)             // TODO: chat sessions may get their own message log submenu
		// in the future. for now, its just disabled
		fDisable = TRUE;
	else {
		if (dat->hwndIEView != 0) {
			mir_snprintf(szTemp, 100, "%s.SRMMEnable", dat->bIsMeta ? dat->szMetaProto : dat->szProto);
			if (DBGetContactSettingByte(NULL, "IEVIEW", szTemp, 0)) {
				mir_snprintf(szTemp, 100, "%s.SRMMMode", dat->bIsMeta ? dat->szMetaProto : dat->szProto);
				IEViewMode = DBGetContactSettingByte(NULL, "IEVIEW", szTemp, 0);
			}
			else
				IEViewMode = DBGetContactSettingByte(NULL, "IEVIEW", "_default_.SRMMMode", 0);
			fDisable = (IEViewMode == 2);
		}
		else if (dat->hwndHPP)
			fDisable = TRUE;
	}
	if (dat->pContainer->hMenu) {
		EnableMenuItem(dat->pContainer->hMenu, 2, MF_BYPOSITION | (fDisable ? MF_GRAYED | MF_DISABLED : MF_ENABLED));
		if (!(dat->pContainer->dwFlags & CNT_NOMENUBAR))
			DrawMenuBar(dat->pContainer->hwnd);
	}
	if (dat->bType == SESSIONTYPE_IM)
		EnableWindow(GetDlgItem(hwndDlg, IDC_TIME), fDisable ? FALSE : TRUE);
}
