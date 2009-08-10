/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2009 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * Menu and status bar control(s) for the container window.
 *
 *
 */

#include "commonheaders.h"
static 	WNDPROC OldStatusBarproc = 0;

extern int 		status_icon_list_size;
extern 			StatusIconListNode *status_icon_list;

CMenuBar::CMenuBar(HWND hwndParent, const ContainerWindowData *pContainer)
{
	REBARINFO RebarInfo;
	REBARBANDINFO RebarBandInfo;
	RECT Rc;

	m_pContainer = const_cast<ContainerWindowData *>(pContainer);
	assert(m_pContainer != 0);

	m_hwndRebar = ::CreateWindowEx(WS_EX_TOOLWINDOW,REBARCLASSNAME, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|RBS_VARHEIGHT|RBS_BANDBORDERS|RBS_DBLCLKTOGGLE|RBS_DBLCLKTOGGLE|CCS_NODIVIDER,
								 0, 0, 0, 0, hwndParent, NULL, g_hInst, NULL);

	RebarInfo.cbSize = 	sizeof(REBARINFO);
	RebarInfo.fMask = 	0;
	RebarInfo.himl = 	(HIMAGELIST)NULL;

	::SendMessage(m_hwndRebar ,RB_SETBARINFO, 0, (LPARAM)&RebarInfo);

	RebarBandInfo.cbSize = sizeof(REBARBANDINFO);
	RebarBandInfo.fMask  = RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_COLORS|RBBIM_SIZE|RBBIM_STYLE|RBBIM_TEXT|RBBIM_IDEALSIZE;
	RebarBandInfo.fStyle = RBBS_FIXEDBMP|RBBS_GRIPPERALWAYS|RBBS_CHILDEDGE;
	RebarBandInfo.hbmBack = 0;
	RebarBandInfo.clrBack = 0;

	TBBUTTON TbButton[5];

	m_hwndToolbar = ::CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD|WS_VISIBLE|TBSTYLE_FLAT|TBSTYLE_LIST|CCS_NOPARENTALIGN|CCS_NORESIZE|CCS_NODIVIDER,
								   0, 0, 0, 0, hwndParent, NULL, g_hInst, NULL);

	::SendMessage(m_hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	::ZeroMemory(TbButton, sizeof(TbButton));

	TbButton[0].iBitmap = I_IMAGENONE;
	TbButton[0].iString = (INT_PTR)TranslateT("File");
	TbButton[0].fsState = TBSTATE_ENABLED;
	TbButton[0].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
	TbButton[0].idCommand = 100;

	TbButton[1].iBitmap = I_IMAGENONE;
	TbButton[1].iString = (INT_PTR)TranslateT("View");
	TbButton[1].fsState = TBSTATE_ENABLED;
	TbButton[1].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
	TbButton[1].idCommand = 101;

	TbButton[2].iBitmap = I_IMAGENONE;
	TbButton[2].iString = (INT_PTR)TranslateT("Message Log");
	TbButton[2].fsState = TBSTATE_ENABLED;
	TbButton[2].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
	TbButton[2].idCommand = 102;

	TbButton[3].iBitmap = I_IMAGENONE;
	TbButton[3].iString = (INT_PTR)TranslateT("Container");
	TbButton[3].fsState = TBSTATE_ENABLED;
	TbButton[3].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
	TbButton[3].idCommand = 102;

	TbButton[4].iBitmap = I_IMAGENONE;
	TbButton[4].iString = (INT_PTR)TranslateT("Help");
	TbButton[4].fsState = TBSTATE_ENABLED;
	TbButton[4].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
	TbButton[4].idCommand = 102;

	::SendMessage(m_hwndToolbar, TB_ADDBUTTONS, sizeof(TbButton)/sizeof(TBBUTTON), (LPARAM)&TbButton);

	long Size = SendMessage(m_hwndToolbar, TB_GETBUTTONSIZE, 0, 0);

	::GetWindowRect(m_hwndToolbar, &Rc);

	RebarBandInfo.lpText     = NULL;
	RebarBandInfo.hwndChild  = m_hwndToolbar;
	RebarBandInfo.cxMinChild = 10;
	RebarBandInfo.cyMinChild = HIWORD(Size);
	RebarBandInfo.cx         = 10;

	::SendMessage(m_hwndRebar,RB_INSERTBAND, (WPARAM)-1, (LPARAM)&RebarBandInfo);
}

CMenuBar::~CMenuBar()
{
	::DestroyWindow(m_hwndToolbar);
	::DestroyWindow(m_hwndRebar);
}

const RECT& CMenuBar::getClientRect()
{
	::GetClientRect(m_hwndRebar, &m_rcClient);
	return(m_rcClient);
}

LONG CMenuBar::getHeight() const
{
	return((m_pContainer->dwFlags & CNT_NOMENUBAR) ? 0 : m_rcClient.bottom - m_rcClient.top);
}

static 	int tooltip_active = FALSE;
static 	POINT ptMouse = {0};
RECT 	rcLastStatusBarClick;						// remembers click (down event) point for status bar clicks

LONG_PTR CALLBACK StatusBarSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct ContainerWindowData *pContainer = (struct ContainerWindowData *)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);

	if (OldStatusBarproc == 0) {
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
			SetWindowLongPtr(hwndParent, GWL_STYLE, GetWindowLongPtr(hwndParent, GWL_STYLE) & ~WS_THICKFRAME);
			SetWindowLongPtr(hwndParent, GWL_EXSTYLE, GetWindowLongPtr(hwndParent, GWL_EXSTYLE) & ~WS_EX_APPWINDOW);
			cs->style &= ~SBARS_SIZEGRIP;
			ret = CallWindowProc(OldStatusBarproc, hWnd, msg, wParam, lParam);
			SetWindowLongPtr(hwndParent, GWL_STYLE, GetWindowLongPtr(hwndParent, GWL_STYLE) | WS_THICKFRAME);
			SetWindowLongPtr(hwndParent, GWL_EXSTYLE, GetWindowLongPtr(hwndParent, GWL_EXSTYLE) | WS_EX_APPWINDOW);
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
			int clip = CSkin::m_bClipBorder;

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

			if ((pContainer && pContainer->bSkinned) || M->isAero())
				return 1;
			if (CMimAPI::m_pfnIsThemeActive != 0) {
				if (CMimAPI::m_pfnIsThemeActive() && !M->isAero())
					break;
			}
			GetClientRect(hWnd, &rcClient);
			FillRect((HDC)wParam, &rcClient, GetSysColorBrush(COLOR_3DFACE));
			return 1;
		}
		case WM_PAINT:
			if (!CSkin::m_skinEnabled && !M->isAero())
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
				CSkinItem *item = &SkinItems[ID_EXTBKSTATUSBARPANEL];

				BOOL	fAero = M->isAero();
				HANDLE  hTheme = fAero ? CMimAPI::m_pfnOpenThemeData(hWnd, L"ButtonStyle") : 0;

				GetClientRect(hWnd, &rcClient);

				hbm = CSkin::CreateAeroCompatibleBitmap(rcClient, hdc);
				hbmOld = (HBITMAP)SelectObject(hdcMem, hbm);
				SetBkMode(hdcMem, TRANSPARENT);
				SetTextColor(hdcMem, PluginConfig.skinDefaultFontColor);
				hFontOld = (HFONT)SelectObject(hdcMem, GetStockObject(DEFAULT_GUI_FONT));

				if (pContainer && pContainer->bSkinned)
					CSkin::SkinDrawBG(hWnd, GetParent(hWnd), pContainer, &rcClient, hdcMem);

				for (i = 0; i < (int)nParts; i++) {
					SendMessage(hWnd, SB_GETRECT, (WPARAM)i, (LPARAM)&itemRect);
					if (!item->IGNORED && !fAero)
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
						_MessageWindowData *dat = (_MessageWindowData *)GetWindowLongPtr(pContainer->hwndActive, GWLP_USERDATA);

						if (dat)
							DrawStatusIcons(dat, hdcMem, itemRect, 2);
					}
					else {
						if (hIcon) {
							if (LOWORD(result) > 1) {				// we have a text
								DrawIconEx(hdcMem, itemRect.left + 3, (height / 2 - 8) + itemRect.top, hIcon, 16, 16, 0, 0, DI_NORMAL);
								itemRect.left += 20;
								CSkin::RenderText(hdcMem, hTheme, szText, &itemRect, DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE | DT_NOPREFIX);
							}
							else
								DrawIconEx(hdcMem, itemRect.left + 3, (height / 2 - 8) + itemRect.top, hIcon, 16, 16, 0, 0, DI_NORMAL);
						}
						else {
							itemRect.left += 2;
							itemRect.right -= 2;
							CSkin::RenderText(hdcMem, hTheme, szText, &itemRect, DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
						}
					}
				}
				BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);
				SelectObject(hdcMem, hbmOld);
				DeleteObject(hbm);
				SelectObject(hdcMem, hFontOld);
				DeleteDC(hdcMem);

				if(hTheme)
					CMimAPI::m_pfnCloseThemeData(hTheme);

				EndPaint(hWnd, &ps);
				return 0;
			}
		case WM_CONTEXTMENU: {
			RECT rc;
			POINT pt;
			GetCursorPos(&pt);
			GetWindowRect(GetDlgItem(hWnd, 1001), &rc);
			if (PtInRect(&rc, pt)) {
				SendMessage(PluginConfig.g_hwndHotkeyHandler, DM_TRAYICONNOTIFY, 100, WM_RBUTTONUP);
				return 0;
			}
			break;
		}

		case WM_USER + 101: {
			struct _MessageWindowData *dat = (struct _MessageWindowData *)lParam;
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
					flags = M->GetByte(dat->hContact, current->sid.szModule, buff, current->sid.flags);
				}
				if (!(flags & MBF_HIDDEN))
					list_icons++;
				current = current->next;
			}

			SendMessage(hWnd, WM_SIZE, 0, 0);
			GetWindowRect(hWnd, &rcs);

			statwidths[0] = (rcs.right - rcs.left) - (2 * SB_CHAR_WIDTH + 20) - (35 + ((list_icons) * (PluginConfig.m_smcxicon + 2)));
			statwidths[1] = (rcs.right - rcs.left) - (45 + ((list_icons) * (PluginConfig.m_smcxicon + 2)));
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
					struct _MessageWindowData *dat = (struct _MessageWindowData *)GetWindowLongPtr(pContainer->hwndActive, GWLP_USERDATA);
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

						unsigned int iconNum = (pt.x - rc.left) / (PluginConfig.m_smcxicon + gap);
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
							flags = M->GetByte(dat->hContact, current->sid.szModule, buff, current->sid.flags);
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
							int mtnStatus = (int)M->GetByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, M->GetByte(SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW));
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
							M->GetTString(dat->hContact,dat->szProto,"Topic",&dbv);

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

