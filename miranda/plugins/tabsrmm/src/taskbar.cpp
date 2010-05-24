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
 * (C) 2005-2010 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * Windows 7 taskbar integration
 *
 * - overlay icons
 * - custom taskbar thumbnails for aero peek in tabbed containers
 * - read Windows 7 task bar configuration from the registry.
 */

#include "commonheaders.h"

CTaskbarInteract* Win7Taskbar = 0;

/**
 * set the overlay icon for a task bar button. Used for typing notifications and incoming
 * message indicator.
 *
 * @param 	hwndDlg HWND: 	container window handle
 * @param 	lParam  LPARAM:	icon handle
 * @return	true if icon has been set and taskbar will accept it, false otherwise
 */
bool CTaskbarInteract::setOverlayIcon(HWND hwndDlg, LPARAM lParam) const
{
	if (m_pTaskbarInterface && m_isEnabled && m_fHaveLargeicons) {
		m_pTaskbarInterface->SetOverlayIcon(hwndDlg,(HICON)lParam, NULL);
		return(true);
	}
	return(false);
}

/**
 * check the task bar status for "large icon mode".
 * @return bool: true if large icons are in use, false otherwise
 */
bool CTaskbarInteract::haveLargeIcons()
{
	m_fHaveLargeicons = false;

	if (m_pTaskbarInterface && m_isEnabled) {
		HKEY 	hKey;
		DWORD 	val = 1;
		DWORD	valGrouping = 2;
		DWORD	size = 4;
		DWORD	dwType = REG_DWORD;
		/*
		 * check whether the taskbar is set to show large icons. This is necessary, because the method SetOverlayIcon()
		 * always returns S_OK, but the icon is simply ignored when using small taskbar icons.
		 * also, figure out the button grouping mode.
		 */
		if(::RegOpenKey(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), &hKey) == ERROR_SUCCESS) {
			::RegQueryValueEx(hKey, _T("TaskbarSmallIcons"), 0, &dwType, (LPBYTE)&val, &size);
			size = 4;
			dwType = REG_DWORD;
			::RegQueryValueEx(hKey, _T("TaskbarGlomLevel"), 0, &dwType, (LPBYTE)&valGrouping, &size);
			::RegCloseKey(hKey);
		}
		m_fHaveLargeicons = (val ? false : true);			// small icons in use, revert to default icon feedback
		m_fHaveAlwaysGrouping = (valGrouping == 0 ? true : false);
	}
	return(m_fHaveLargeicons);
}

/**
 * removes the overlay icon for the given container window
 * @param hwndDlg HWND: window handle
 */
void CTaskbarInteract::clearOverlayIcon(HWND hwndDlg) const
{
	if (m_pTaskbarInterface && m_isEnabled)
		m_pTaskbarInterface->SetOverlayIcon(hwndDlg, NULL, NULL);
}

LONG CTaskbarInteract::updateMetrics()
{
	m_IconSize = 32;

	return(m_IconSize);
}

/**
 * register a new task bar button ("tab") for the button group hwndContainer
 * (one of the top level message windows)
 * @param hwndTab			proxy window handle
 * @param hwndContainer		a message container window
 */
void CTaskbarInteract::registerTab(const HWND hwndTab, const HWND hwndContainer) const
{
	if(m_isEnabled) {
		m_pTaskbarInterface->RegisterTab(hwndTab, hwndContainer);
		m_pTaskbarInterface->SetTabOrder(hwndTab, 0);
		m_pTaskbarInterface->SetThumbnailTooltip(hwndContainer, _T(""));
	}
}

/**
 * remove a previously registered proxy window. The destructor of the proxy
 * window class is using this before destroying the proxy window itself.
 * @param hwndTab	proxy window handle
 */
void CTaskbarInteract::unRegisterTab(const HWND hwndTab) const
{
	if(m_isEnabled)
		m_pTaskbarInterface->UnregisterTab(hwndTab);
}

void CTaskbarInteract::SetTabActive(const HWND hwndTab) const
{
	if(m_isEnabled)
		m_pTaskbarInterface->SetTabActive(hwndTab, m_hwndClist, 0);
}

void CTaskbarInteract::setThumbnailClip(const HWND hwndTab, const RECT* rc) const
{
	if(m_isEnabled)
		m_pTaskbarInterface->SetThumbnailClip(hwndTab, const_cast<RECT *>(rc));
}

/**
 * create a proxy window object for the given session. Do NOT call this more than once
 * per session and not outside of WM_INITDIALOG after most things are initialized.
 * @param dat		session window data
 *
 * static member function
 */
void CProxyWindow::add(TWindowData *dat)
{
	if(IsWinVer7Plus() && PluginConfig.m_useAeroPeek && M->isDwmActive())
		dat->pWnd = new CProxyWindow(dat);
	else
		dat->pWnd = 0;
}

/**
 * This is called from the broadcasted WM_DWMCOMPOSITIONCHANGED event by all messages
 * sessions. It checks and, if needed, destroys or creates a proxy object, based on
 * the status of the DWM
 * @param dat		session window data
 *
 * static member function
 */
void CProxyWindow::verify(TWindowData *dat)
{
	if(M->isDwmActive()) {
		if(0 == dat->pWnd) {
			dat->pWnd = new CProxyWindow(dat);
			if(dat->pWnd) {
				dat->pWnd->updateIcon(dat->hTabStatusIcon);
				dat->pWnd->updateTitle(dat->cache->getNick());
			}
		}
	}
	else {
		if(dat->pWnd) {
			delete dat->pWnd;
			dat->pWnd = 0;
		}
	}
}

/**
 * create the proxy (toplevel) window required to show per tab thumbnails
 * and previews for a message session.
 * each tab has one invisible proxy window
 */
CProxyWindow::CProxyWindow(const TWindowData *dat)
{
	m_dat = dat;
	m_hwnd = ::CreateWindowEx(/*WS_EX_TOOLWINDOW | */WS_EX_NOACTIVATE, PROXYCLASSNAME, _T(""),
		WS_POPUP | WS_BORDER | WS_SYSMENU | WS_CAPTION, -32000, -32000, 10, 10, NULL, NULL, g_hInst, (LPVOID)this);

	m_hBigIcon = 0;

#if defined(__LOGDEBUG_)
	_DebugTraceW(_T("create proxy object for: %s"), m_dat->cache->getNick());
#endif
	Win7Taskbar->registerTab(m_hwnd, m_dat->pContainer->hwnd);
	if(CMimAPI::m_pfnDwmSetWindowAttribute) {
		BOOL	fIconic = TRUE;
		BOOL	fHasIconicBitmap = TRUE;
		HWND	hwndSrc = m_hwnd;

		CMimAPI::m_pfnDwmSetWindowAttribute(m_hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &fIconic,  sizeof(fIconic));
		CMimAPI::m_pfnDwmSetWindowAttribute(m_hwnd, DWMWA_HAS_ICONIC_BITMAP, &fHasIconicBitmap, sizeof(fHasIconicBitmap));
	}
}

CProxyWindow::~CProxyWindow()
{
	Win7Taskbar->unRegisterTab(m_hwnd);
	::DestroyWindow(m_hwnd);

#if defined(__LOGDEBUG_)
	_DebugTraceW(_T("destroy proxy object for: %s"), m_dat->cache->getNick());
#endif
	if(m_hbmThumb)
		::DeleteObject(m_hbmThumb);
}

/**
 * refresh thumbnail image
 * whenever the DWM requests a thumbnail this might create one, either
 * if none does yet exist, the size has changed or the Invalidate() was
 * called since the last update.
 */
void CProxyWindow::refreshThumb()
{
	RECT			rc = {0};
	HBITMAP 		hbmOld = 0;
	HBRUSH  		brBack = 0;
	LONG			lIconSize = 32, cx, cy;
	const TCHAR*	tszStatusMsg = m_dat->cache->getStatusMsg();
	HBITMAP			hbmAvatar, hbmOldAv;
	double			dNewWidth = 0.0, dNewHeight = 0.0;
	bool			fFree = false;
	HRGN			hRgn = 0;
	BOOL			fUnread = m_dat->dwUnread ? true : false;
	HICON			hIcon;
	HFONT			hOldFont = 0;
	SIZE			sz = {0};
	DWORD			dtFlags = 0;

	if(m_hbmThumb)
		::DeleteObject(m_hbmThumb);

#if defined(__LOGDEBUG_)
	_DebugTraceW(_T("recreating thumb for %s"), m_dat->cache->getNick());
#endif

	rc.right = m_width;
	rc.bottom = m_height;

	HDC dc = ::GetDC(m_hwnd);
	HDC	hdc = ::CreateCompatibleDC(dc);

	m_hbmThumb = CSkin::CreateAeroCompatibleBitmap(rc, hdc);
	hbmOld = reinterpret_cast<HBITMAP>(::SelectObject(hdc, m_hbmThumb));
	ReleaseDC(m_hwnd, dc);

	brBack = ::CreateSolidBrush(fUnread ? RGB(70, 60, 60) : RGB(60, 60, 60));
	::FillRect(hdc, &rc, brBack);

	::SelectObject(hdc, hbmOld);
	CImageItem::SetBitmap32Alpha(m_hbmThumb, fUnread ? 100 : 80);
	hbmOld = reinterpret_cast<HBITMAP>(::SelectObject(hdc, m_hbmThumb));

	SetBkMode(hdc, TRANSPARENT);

	hOldFont = reinterpret_cast<HFONT>(::SelectObject(hdc, CInfoPanel::m_ipConfig.hFonts[IPFONTID_STATUS]));
	::GetTextExtentPoint32A(hdc, "A", 1, &sz);

	rc.left += 3;
	rc.right -= 3;

	RECT rcTop = rc;
	RECT rcBottom = rc;
	rcBottom.top = rc.bottom - ( 2 * (rcBottom.bottom / 5)) - 2;
	rcTop.bottom = rcBottom.top - 2;

	RECT rcIcon = rcTop;
	rcIcon.right = rc.right / 3;

	hIcon = m_hBigIcon;
	if(0 == hIcon) {
		if(fUnread) {
			if(PluginConfig.g_IconMsgEventBig)
				hIcon = PluginConfig.g_IconMsgEventBig;
			else {
				hIcon = PluginConfig.g_IconMsgEvent;
				lIconSize = 16;
			}
		}
		else {
			hIcon = reinterpret_cast<HICON>(LoadSkinnedProtoIconBig(m_dat->cache->getActiveProto(), m_dat->cache->getActiveStatus()));
			if(0 == hIcon || reinterpret_cast<HICON>(CALLSERVICE_NOTFOUND) == hIcon) {
				hIcon = reinterpret_cast<HICON>(LoadSkinnedProtoIcon(m_dat->cache->getActiveProto(), m_dat->cache->getActiveStatus()));
				lIconSize = 16;
			}
		}
	}
	::DrawIconEx(hdc, rcIcon.right / 2 - lIconSize / 2, rcIcon.top + 3, hIcon, lIconSize, lIconSize, 0, 0, DI_NORMAL);

	rcIcon.top += (lIconSize + 7);
	CSkin::RenderText(hdc, m_dat->hThemeToolbar, m_dat->szStatus, &rcIcon, 0x80000000 | DT_CENTER | DT_WORD_ELLIPSIS, 10);
	if(fUnread) {
		TCHAR	tszTemp[30];

		rcIcon.top += sz.cy;
		mir_sntprintf(tszTemp, 30, _T("%d Unread"), m_dat->dwUnread);
		CSkin::RenderText(hdc, m_dat->hThemeToolbar, tszTemp, &rcIcon, 0x80000000 | DT_CENTER | DT_WORD_ELLIPSIS, 10);
	}

	/*
	 * avatar
	 */

	rcIcon= rcTop;
	rcIcon.top += 2;
	rcIcon.left = rc.right / 3;
	cx = rcIcon.right - rcIcon.left;
	cy = rcIcon.bottom - rcIcon.top;

	hbmAvatar = (m_dat->ace && m_dat->ace->hbmPic) ? m_dat->ace->hbmPic : PluginConfig.g_hbmUnknown;
	Utils::scaleAvatarHeightLimited(hbmAvatar, dNewWidth, dNewHeight, rcIcon.bottom - rcIcon.top);

	HBITMAP hbmResized = CSkin::ResizeBitmap(hbmAvatar, dNewWidth, dNewHeight, fFree);

	dc = CreateCompatibleDC(hdc);
	hbmOldAv = reinterpret_cast<HBITMAP>(::SelectObject(dc, hbmResized));

	LONG xOff = rcIcon.right - (LONG)dNewWidth - 2;
	LONG yOff = (cy - (LONG)dNewHeight) / 2 + rcIcon.top;

	hRgn = ::CreateRectRgn(xOff - 1, yOff - 1, xOff + (LONG)dNewWidth + 2, yOff + (LONG)dNewHeight + 2);
	CSkin::m_default_bf.SourceConstantAlpha = 150;
	CMimAPI::m_MyAlphaBlend(hdc, xOff, yOff, (LONG)dNewWidth, (LONG)dNewHeight, dc, 0, 0, (LONG)dNewWidth, (LONG)dNewHeight, CSkin::m_default_bf);
	CSkin::m_default_bf.SourceConstantAlpha = 255;
	::FrameRgn(hdc, hRgn, reinterpret_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)), 1, 1);

	::DeleteObject(hRgn);
	::SelectObject(dc, hbmOldAv);
	::DeleteObject(hbmResized);
	::DeleteDC(dc);

	rcBottom.bottom -= 16;
	/*
	 * status message and bottom line (either UID or nick name, depending on
	 * task bar grouping mode).
	 */
	if((rcBottom.bottom - rcBottom.top) < 2 * sz.cy)
		dtFlags |= DT_SINGLELINE;

	rcBottom.bottom -= ((rcBottom.bottom - rcBottom.top) % sz.cy);		// adjust to a multiple of line height

	if(0 == tszStatusMsg)
		tszStatusMsg = CTranslator::get(CTranslator::GEN_NO_STATUS);
	CSkin::RenderText(hdc, m_dat->hThemeToolbar, tszStatusMsg, &rcBottom, 0x80000000 | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | dtFlags, 10);
	rcBottom.bottom = rc.bottom;
	rcBottom.top = rcBottom.bottom - sz.cy - 2;
	CSkin::RenderText(hdc, m_dat->hThemeToolbar, Win7Taskbar->haveAlwaysGroupingMode() ? m_dat->cache->getUIN() : m_dat->cache->getNick(),
					  &rcBottom, 0x80000000 | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS, 10);

	if(hOldFont)
		::SelectObject(hdc, hOldFont);

	::SelectObject(hdc, hbmOld);
	::DeleteDC(hdc);
	::DeleteObject(brBack);
}

/**
 * send a thumbnail to the DWM. If required, refresh it first.
 * called by WM_DWMSENDICONICTHUMBNAIL handler.
 *
 * @param width		thumbnail width as requested by DWM via lParam
 * @param height	thumbnail height as requested by DWM via lParam
 */
void CProxyWindow::sendThumb(LONG width, LONG height)
{
	if(width != m_width || height != m_height || m_hbmThumb == 0) {
		m_width = width;
		m_height = height;
		refreshThumb();
	}
	CMimAPI::m_pfnDwmSetIconicThumbnail(m_hwnd, m_hbmThumb, DWM_SIT_DISPLAYFRAME);
}

/**
 * send a live preview image of a given message session to the DWM.
 * called by WM_DWMSENDICONICLIVEPREVIEWBITMAP on DWM's request.
 *
 * The bitmap can be deleted after submitting it, because the DWM
 * will cache a copy of it (and re-request it when its own bitmap cache
 * was purged).
 */
void CProxyWindow::sendPreview()
{
	POINT 	pt = {0};
	RECT	rcContainer, rcBox;
	HDC		hdc, dc;

#if defined(__LOGDEBUG_)
	_DebugTraceW(_T("recreating preview for %s"), m_dat->cache->getNick());
#endif

	/*
	 * a minimized container has a null rect as client area, so do not use it
	 * use the last known client area size instead.
	 */
	if(!IsIconic(m_dat->pContainer->hwnd))
		::GetClientRect(m_dat->pContainer->hwnd, &rcContainer);
	else
		rcContainer = m_dat->pContainer->rcSaved;

#if defined(__LOGDEBUG_)
	_DebugTraceW(_T("client rect is: %d, %d - saved = %d, %d"), rcContainer.right, rcContainer.bottom, m_dat->pContainer->rcSaved.right, m_dat->pContainer->rcSaved.bottom);
#endif

	dc = ::GetDC(m_dat->hwnd);
	hdc = ::CreateCompatibleDC(dc);
	HBITMAP hbm = CSkin::CreateAeroCompatibleBitmap(rcContainer, hdc);
	HBITMAP hbmOld = reinterpret_cast<HBITMAP>(::SelectObject(hdc, hbm));

	::DrawAlpha(hdc, &rcContainer, PluginConfig.m_ipBackgroundGradient, 50, PluginConfig.m_ipBackgroundGradientHigh, 0, 17, 0, 2, 0);
	CImageItem::SetBitmap32Alpha(hbm, 220);

	HDC dcSrc = ::CreateCompatibleDC(hdc);
	HBITMAP hOldSrc = reinterpret_cast<HBITMAP>(::SelectObject(dcSrc, m_hbmThumb));

	rcBox.left = (rcContainer.right - m_width) / 2 - 20;
	rcBox.right = rcBox.left + m_width + 40;
	rcBox.top = (rcContainer.bottom - m_height) / 2 - 20;
	rcBox.bottom = rcBox.top + m_height + 40;

	::DrawAlpha(hdc, &rcBox, 0x00ffffff, 60, 0x00ffffff, 0, 0, CORNER_ALL, 12, 0);
	CMimAPI::m_MyAlphaBlend(hdc, rcBox.left + 20, rcBox.top + 20, m_width, m_height,
							dcSrc, 0, 0, m_width, m_height, CSkin::m_default_bf);

	::SelectObject(dcSrc, hOldSrc);
	::DeleteDC(dcSrc);

	rcContainer.top = rcContainer.bottom - 20;
	CSkin::RenderText(hdc, m_dat->hThemeToolbar, _T("Placeholder, not fully implemented"), &rcContainer, DT_CENTER | DT_SINGLELINE, 10);
	//StretchBlt(hdc, 0, 0, rcContainer.right, rcContainer.bottom, dc, 0, 0, rcClient.right, rcClient.bottom, SRCCOPY | CAPTUREBLT);
	::SelectObject(hdc, hbmOld);
	::DeleteDC(hdc);
	CMimAPI::m_pfnDwmSetIconicLivePreviewBitmap(m_hwnd, hbm, &pt, DWM_SIT_DISPLAYFRAME);
	::ReleaseDC(m_dat->hwnd, dc);
	::DeleteObject(hbm);
}

/**
 * set the larg icon for the thumbnail. This is mostly used by group chats
 * to indicate last active event in the session.
 *
 * hIcon may be 0 to remove a custom big icon. In that case, the renderer
 * will try to figure out a suitable one, based on session data.
 *
 * @param hIcon			icon handle (should be a 32x32 icon)
 * @param fInvalidate	invalidate the thumbnail (default value = true)
 */
void CProxyWindow::setBigIcon(const HICON hIcon, bool fInvalidate)
{
	m_hBigIcon = hIcon;
	if(fInvalidate)
		Invalidate();
}

/**
 * update the (small) thumbnail icon in front of the title string
 * @param hIcon		new icon handle
 */
void CProxyWindow::updateIcon(const HICON hIcon) const
{
	if(m_hwnd && hIcon)
		::SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
}

/**
 * set the task bar button ("tab") active.
 */
void CProxyWindow::activateTab() const
{
	Win7Taskbar->SetTabActive(m_hwnd);
}
/**
 * invalidate the thumbnail, it will be recreated at the next request
 * by the DWM
 *
 * this is called from several places whenever a relevant information,
 * represented in a thumbnail image, has changed.
 *
 * also tell the DWM that it must request a new thumb.
 */
void CProxyWindow::Invalidate()
{
	if(m_hbmThumb) {
		::DeleteObject(m_hbmThumb);
		m_hbmThumb = 0;
	}
	/*
	 * tell the DWM to request a new thumbnail for the proxy window m_hwnd
	 * when it needs one.
	 */
	CMimAPI::m_pfnDwmInvalidateIconicBitmaps(m_hwnd);
}

/**
 * update the thumb title string (usually, the nickname)
 * @param tszTitle: 	new title string
 */
void CProxyWindow::updateTitle(const TCHAR *tszTitle) const
{
	if(m_hwnd && tszTitle)
		::SetWindowText(m_hwnd, tszTitle);
}
/**
 * stub window procedure for the custom proxy window class
 * just initialize GWLP_USERDATA and call the object's method
 *
 * static member function
 */
LRESULT CALLBACK CProxyWindow::stubWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CProxyWindow* pWnd = reinterpret_cast<CProxyWindow *>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));

	if(pWnd)
		return(pWnd->wndProc(hWnd, msg, wParam, lParam));

	switch(msg) {
		case WM_NCCREATE: {
			CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
			CProxyWindow *pWnd = reinterpret_cast<CProxyWindow *>(cs->lpCreateParams);
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<UINT_PTR>(pWnd));
			return(pWnd->wndProc(hWnd, msg, wParam, lParam));
		}
		default:
			break;
	}
	return(::DefWindowProc(hWnd, msg, wParam, lParam));
}

/**
 * window procedure for the proxy window
 */
LRESULT CALLBACK CProxyWindow::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {

#if defined(__LOGDEBUG_)
		case WM_NCCREATE:
			_DebugTraceW(_T("create proxy WINDOW for: %s"), m_dat->cache->getNick());
			break;
#endif
			/*
			 * proxy window was activated by clicking on the thumbnail. Send this
			 * to the real message window.
			 */
		case WM_ACTIVATE:
			::SendMessage(m_dat->hwnd, DM_ACTIVATEME, 0, 0);
			::SetFocus(m_dat->hwnd);
			return(0);			// no default processing, avoid flickering.

		case WM_NCDESTROY:
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
#if defined(__LOGDEBUG_)
			_DebugTraceW(_T("destroy proxy WINDOW for: %s"), m_dat->cache->getNick());
#endif
			break;

        case WM_DWMSENDICONICTHUMBNAIL:
        	sendThumb(HIWORD(lParam), LOWORD(lParam));
        	return(0);

        case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
       		sendPreview();
            return(0);

        case WM_CLOSE:
        	::SendMessage(m_dat->hwnd, WM_CLOSE, 1, 2);
        	break;

		default:
			break;
	}
	return(::DefWindowProc(hWnd, msg, wParam, lParam));
}
