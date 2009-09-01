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
 */

#include "commonheaders.h"
static 	WNDPROC OldStatusBarproc = 0;

extern int 		status_icon_list_size;
extern 			StatusIconListNode *status_icon_list;

bool	 	CMenuBar::m_buttonsInit = false;
HHOOK		CMenuBar::m_hHook = 0;
TBBUTTON 	CMenuBar::m_TbButtons[8] = {0};
CMenuBar	*CMenuBar::m_Owner = 0;
HBITMAP		CMenuBar::m_MimIcon = 0;
int			CMenuBar::m_MimIconRefCount = 0;

CMenuBar::CMenuBar(HWND hwndParent, const ContainerWindowData *pContainer)
{
	//REBARINFO RebarInfo;
	//REBARBANDINFO RebarBandInfo;
	RECT Rc;

	m_pContainer = const_cast<ContainerWindowData *>(pContainer);

	if(m_MimIcon == 0) {
		HDC	hdc = ::GetDC(m_pContainer->hwnd);
		HICON hIcon = LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);

		HDC hdcTemp = ::CreateCompatibleDC(hdc);

		RECT rc = {0,0,16,16};
		m_MimIcon = CSkin::CreateAeroCompatibleBitmap(rc, hdcTemp);
		HBITMAP hbmOld = (HBITMAP)::SelectObject(hdcTemp, m_MimIcon);
		::DrawIconEx(hdcTemp, 0, 0, hIcon, 16, 16, 0, 0, DI_NORMAL);
		::SelectObject(hdcTemp, hbmOld);
		::DeleteDC(hdcTemp);
		::ReleaseDC(m_pContainer->hwnd, hdc);
	}

	m_MimIconRefCount++;

	//m_hwndRebar = ::CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|RBS_AUTOSIZE|/*RBS_BANDBORDERS|*/RBS_DBLCLKTOGGLE,
	//							   0, 0, 0, 0, hwndParent, NULL, g_hInst, NULL);

	/*RebarInfo.cbSize = 	sizeof(REBARINFO);
	RebarInfo.fMask = 	0;
	RebarInfo.himl = 	(HIMAGELIST)NULL;

	::SendMessage(m_hwndRebar, RB_SETBARINFO, 0, (LPARAM)&RebarInfo);

	RebarBandInfo.cbSize = sizeof(REBARBANDINFO);
	RebarBandInfo.fMask  = RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_STYLE|RBBIM_IDEALSIZE;
	RebarBandInfo.fStyle = RBBS_FIXEDBMP|RBBS_TOPALIGN|RBBS_NOGRIPPER;//|RBBS_GRIPPERALWAYS;
	RebarBandInfo.hbmBack = 0;
	RebarBandInfo.clrBack = GetSysColor(COLOR_3DFACE);*/
	m_hwndToolbar = ::CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL, WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VISIBLE|TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|TBSTYLE_LIST|/*CCS_NOPARENTALIGN|*/CCS_NODIVIDER|CCS_TOP,
								   0, 0, 0, 0, hwndParent, NULL, g_hInst, NULL);

	::SendMessage(m_hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	if(m_buttonsInit == false) {
		::ZeroMemory(m_TbButtons, sizeof(m_TbButtons));

		m_TbButtons[0].iBitmap = 0;//I_IMAGENONE;
		m_TbButtons[0].iString = 0;//(INT_PTR)TranslateT("&Main");
		m_TbButtons[0].fsState = TBSTATE_ENABLED;
		m_TbButtons[0].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
		m_TbButtons[0].idCommand = 100;
		m_TbButtons[0].dwData = 0;

		m_TbButtons[1].iBitmap = I_IMAGENONE;
		m_TbButtons[1].iString = (INT_PTR)TranslateT("&File");
		m_TbButtons[1].fsState = TBSTATE_ENABLED;
		m_TbButtons[1].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
		m_TbButtons[1].idCommand = 101;
		m_TbButtons[1].dwData = reinterpret_cast<DWORD_PTR>(::GetSubMenu(PluginConfig.getMenuBar(), 0));

		m_TbButtons[2].iBitmap = I_IMAGENONE;
		m_TbButtons[2].iString = (INT_PTR)TranslateT("&View");
		m_TbButtons[2].fsState = TBSTATE_ENABLED;
		m_TbButtons[2].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
		m_TbButtons[2].idCommand = 102;
		m_TbButtons[2].dwData = reinterpret_cast<DWORD_PTR>(::GetSubMenu(PluginConfig.getMenuBar(), 1));

		m_TbButtons[3].iBitmap = I_IMAGENONE;
		m_TbButtons[3].iString = (INT_PTR)TranslateT("&User");
		m_TbButtons[3].fsState = TBSTATE_ENABLED;
		m_TbButtons[3].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
		m_TbButtons[3].idCommand = 103;
		m_TbButtons[3].dwData = 0;								// dynamically built by Clist service

		m_TbButtons[4].iBitmap = I_IMAGENONE;
		m_TbButtons[4].iString = (INT_PTR)TranslateT("Room");
		m_TbButtons[4].fsState = TBSTATE_ENABLED;
		m_TbButtons[4].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
		m_TbButtons[4].idCommand = 104;
		m_TbButtons[4].dwData = 0;

		m_TbButtons[5].iBitmap = I_IMAGENONE;
		m_TbButtons[5].iString = (INT_PTR)TranslateT("Message &Log");
		m_TbButtons[5].fsState = TBSTATE_ENABLED;
		m_TbButtons[5].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
		m_TbButtons[5].idCommand = 105;
		m_TbButtons[5].dwData = reinterpret_cast<DWORD_PTR>(::GetSubMenu(PluginConfig.getMenuBar(), 2));

		m_TbButtons[6].iBitmap = I_IMAGENONE;
		m_TbButtons[6].iString = (INT_PTR)TranslateT("&Container");
		m_TbButtons[6].fsState = TBSTATE_ENABLED;
		m_TbButtons[6].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
		m_TbButtons[6].idCommand = 106;
		m_TbButtons[6].dwData = reinterpret_cast<DWORD_PTR>(::GetSubMenu(PluginConfig.getMenuBar(), 3));

		m_TbButtons[7].iBitmap = I_IMAGENONE;
		m_TbButtons[7].iString = (INT_PTR)TranslateT("Help");
		m_TbButtons[7].fsState = TBSTATE_ENABLED;
		m_TbButtons[7].fsStyle = BTNS_DROPDOWN|BTNS_AUTOSIZE;
		m_TbButtons[7].idCommand = 107;
		m_TbButtons[7].dwData = reinterpret_cast<DWORD_PTR>(::GetSubMenu(PluginConfig.getMenuBar(), 4));

		m_buttonsInit = true;
	}

	::SendMessage(m_hwndToolbar, TB_ADDBUTTONS, sizeof(m_TbButtons)/sizeof(TBBUTTON), (LPARAM)m_TbButtons);
	::SendMessage(m_hwndToolbar, TB_SETWINDOWTHEME, 0, (LPARAM)L"REBAR");
	m_size_y = HIWORD(SendMessage(m_hwndToolbar, TB_GETBUTTONSIZE, 0, 0));

	::GetWindowRect(m_hwndToolbar, &Rc);

	TBADDBITMAP tb;
	tb.nID = (UINT_PTR)m_MimIcon;
	tb.hInst = 0;

	::SendMessage(m_hwndToolbar, TB_ADDBITMAP, 1, (LPARAM)&tb);

	/*RebarBandInfo.lpText     = NULL;
	RebarBandInfo.hwndChild  = m_hwndToolbar;
	RebarBandInfo.cxMinChild = 10;
	RebarBandInfo.cyMinChild = m_size_y;
	RebarBandInfo.cx         = 2000;

	::SendMessage(m_hwndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&RebarBandInfo);
	*/
	m_activeMenu = 0;
	m_activeID = 0;
	m_isAero = M->isAero();
	m_mustAutoHide = false;
	m_activeSubMenu = 0;
	m_fTracking = false;
	m_isContactMenu = m_isMainMenu = false;
}

CMenuBar::~CMenuBar()
{
	::DestroyWindow(m_hwndToolbar);
	//::DestroyWindow(m_hwndRebar);
	releaseHook();
	m_MimIconRefCount--;
	if(m_MimIconRefCount == 0) {
		::DeleteObject(m_MimIcon);
		m_MimIcon = 0;
	}
}

/**
 * retrieves the client rectangle for the rebar control. This must be
 * called once per WM_SIZE event by the parent window. getHeight() depends on it.
 *
 * @return RECT&: client rectangle of the rebar control
 */
const RECT& CMenuBar::getClientRect()
{
	::GetClientRect(m_hwndToolbar, &m_rcClient);
	return(m_rcClient);
}

void CMenuBar::obtainHook()
{
	releaseHook();
	if(m_hHook == 0)
		m_hHook = ::SetWindowsHookEx(WH_MSGFILTER, CMenuBar::MessageHook, g_hInst, 0);
	m_Owner = this;
}

void CMenuBar::releaseHook()
{
	if(m_hHook) {
		UnhookWindowsHookEx(m_hHook);
		m_hHook = 0;
	}
}
/**
 * Retrieve the height of the rebar control
 *
 * @return LONG: height of the rebar, in pixels
 */
LONG CMenuBar::getHeight() const
{
	return((m_pContainer->dwFlags & CNT_NOMENUBAR) ? 0 : m_size_y + 2);
}

/**
 * process all relevant messages. Must be called by the parent window's
 * window procedure.
 *
 * @param msg
 * @param wParam
 * @param lParam
 *
 * @return LRESULT: message processing result. Win32 conform.
 *         -1 means: nothing processed, caller should continue as usual.
 */
LONG_PTR CMenuBar::processMsg(const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
	if(msg == WM_NOTIFY) {
		NMHDR* pNMHDR = (NMHDR*)lParam;
		switch(pNMHDR->code) {
			case NM_CUSTOMDRAW: {
				NMCUSTOMDRAW *nm = (NMCUSTOMDRAW*)lParam;
				return(customDrawWorker(nm));
			}

			case TBN_DROPDOWN: {
				NMTOOLBAR *mtb = (NMTOOLBAR *)lParam;

				LRESULT result = Handle(mtb);
				return(result);
			}
			case TBN_HOTITEMCHANGE: {
				NMTBHOTITEM *nmtb = (NMTBHOTITEM *)lParam;

				if(nmtb->idNew != 0 && m_fTracking && nmtb->idNew != m_activeID && m_activeID != 0) {
					cancel(0);
					return(0);
				}
				else if(m_fTracking == true && m_activeID == 0 && nmtb->idNew != 0) {
					invoke(nmtb->idNew);
					return(0);
				}
				break;
			}
			default:
				return(-1);
		}
	}
	else if(msg == WM_LBUTTONDOWN) {
		if (m_pContainer->dwFlags & CNT_NOTITLE) {
			POINT	pt;

			GetCursorPos(&pt);
			return SendMessage(m_pContainer->hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(pt.x, pt.y));
		}
	}
	return(-1);
}
/**
 * Implements NM_CUSTOMDRAW for the rebar
 *
 * @param nm     NMCUSTOMDRAW *: sent via NM_CUSTOMDRAW message
 *
 * @return LONG_PTR: see Win32 NM_CUSTOMDRAW message. The function must return a valid
 *         message return value to indicate how Windows should continue with the drawing process.
 *
 *         It may return zero in which case, the caller should allow default processing for
 *         the NM_CUSTOMDRAW message.
 */
LONG_PTR CMenuBar::customDrawWorker(NMCUSTOMDRAW *nm)
{
	bool fMustDraw = (m_pContainer->bSkinned || m_isAero || M->isVSThemed());

	if(nm->hdr.hwndFrom == m_hwndToolbar) {
		NMTBCUSTOMDRAW *nmtb = (NMTBCUSTOMDRAW *)(nm);

		switch(nmtb->nmcd.dwDrawStage) {
			case CDDS_PREPAINT:
				if(fMustDraw) {
					if(nmtb->nmcd.dwItemSpec == 0) {
						m_hdcDraw = ::CreateCompatibleDC(nmtb->nmcd.hdc);
						::GetClientRect(m_hwndToolbar, &m_rcItem);
						m_hbmDraw = CSkin::CreateAeroCompatibleBitmap(m_rcItem, nmtb->nmcd.hdc);
						m_hbmOld = reinterpret_cast<HBITMAP>(::SelectObject(m_hdcDraw, m_hbmDraw));
						m_hTheme = M->isAero() || M->isVSThemed() ? CMimAPI::m_pfnOpenThemeData(m_hwndToolbar, L"REBAR") : 0;
						m_hOldFont = (HFONT)::SelectObject(m_hdcDraw, (HFONT)::GetStockObject(DEFAULT_GUI_FONT));
						if(m_isAero) {
							nm->rc.bottom--;
							CSkin::ApplyAeroEffect(m_hdcDraw, &m_rcItem, CSkin::AERO_EFFECT_AREA_MENUBAR);
							nm->rc.bottom++;
						}
						else if(M->isVSThemed()) {
							m_rcItem.bottom -= 2;
							M->m_pfnDrawThemeBackground(m_hTheme, m_hdcDraw, 6, 1, &m_rcItem, &m_rcItem);
						}
						else if(CSkin::m_MenuBGBrush)
							::FillRect(m_hdcDraw, &nm->rc, CSkin::m_MenuBGBrush);
						else
							::FillRect(m_hdcDraw, &nm->rc, GetSysColorBrush(COLOR_3DFACE));
					}
					return(CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYPOSTERASE);
				}
				else {
					m_hdcDraw = 0;
					return(CDRF_DODEFAULT);
				}

			case CDDS_ITEMPREPAINT:
				if(fMustDraw) {
					TCHAR	*szText = 0;

					int iIndex = idToIndex(nmtb->nmcd.dwItemSpec);

					if(iIndex >= 0 && iIndex < NR_BUTTONS)
						szText = reinterpret_cast<TCHAR *>(m_TbButtons[iIndex].iString);

					UINT	uState = nmtb->nmcd.uItemState;

					nmtb->nmcd.rc.bottom--;
					if(m_isAero || M->isVSThemed()) {
						COLORREF clr = GetSysColor(COLOR_HOTLIGHT);
						COLORREF clrRev = clr; //RGB(GetBValue(clr), GetGValue(clr), GetRValue(clr));
						if(uState & CDIS_MARKED || uState & CDIS_CHECKED) {
							::DrawAlpha(m_hdcDraw, &nmtb->nmcd.rc, clrRev, 80, clrRev, 0, 9,
										31, 4, 0);
						}
						if(uState & CDIS_SELECTED) {
							::DrawAlpha(m_hdcDraw, &nmtb->nmcd.rc, clrRev, 80, clrRev, 0, 9,
										31, 4, 0);
						}
						if(uState & CDIS_HOT) {
							::DrawAlpha(m_hdcDraw, &nmtb->nmcd.rc, clrRev, 80, clrRev, 0, 9,
										31, 4, 0);
						}
					}

					if(szText) {
						::SetBkColor(m_hdcDraw, (uState & (CDIS_SELECTED | CDIS_HOT | CDIS_MARKED)) ? ::GetSysColor(COLOR_HIGHLIGHTTEXT) : ::GetSysColor(COLOR_BTNTEXT));
						::SetBkMode(m_hdcDraw, TRANSPARENT);
						CSkin::RenderText(m_hdcDraw, m_hTheme, szText, &nmtb->nmcd.rc, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
					}
					if(iIndex == 0) {
						::DrawIconEx(m_hdcDraw, (nmtb->nmcd.rc.left + nmtb->nmcd.rc.right) / 2 - 8,
									 (nmtb->nmcd.rc.top + nmtb->nmcd.rc.bottom) / 2 - 8, LoadSkinnedIcon(SKINICON_OTHER_MIRANDA),
									 16, 16, 0, 0, DI_NORMAL);
					}
					return(CDRF_SKIPDEFAULT);
				}
				else
					return(CDRF_DODEFAULT);

			case CDDS_PREERASE:
			case CDDS_ITEMPOSTERASE:
			case CDDS_ITEMPOSTPAINT:
			case CDDS_ITEMPREERASE:
				return(fMustDraw ? CDRF_SKIPDEFAULT : CDRF_DODEFAULT);

			case CDDS_POSTERASE:
				return(fMustDraw ? CDRF_SKIPDEFAULT : CDRF_DODEFAULT);

			case CDDS_POSTPAINT:
				if(nmtb->nmcd.dwItemSpec == 0 && m_hdcDraw) {
					::BitBlt(nmtb->nmcd.hdc, 0, 0, m_rcItem.right - m_rcItem.left, m_rcItem.bottom - m_rcItem.top,
							 m_hdcDraw, 0, 0, SRCCOPY);
					::SelectObject(m_hdcDraw, m_hbmOld);
					::DeleteObject(m_hbmDraw);
					::SelectObject(m_hdcDraw, m_hOldFont);
					::DeleteDC(m_hdcDraw);
					m_hdcDraw = 0;
					if(m_hTheme)
						CMimAPI::m_pfnCloseThemeData(m_hTheme);
					return(CDRF_SKIPDEFAULT);
				}
				else
					return(CDRF_DODEFAULT);

			default:
				return(CDRF_DODEFAULT);
		}

		return(0);
	}
	return(0);
}

/**
 * Handle the TBN_DROPDOWN notification message sent by the
 * toolbar control.
 *
 * @param nmtb   NMTOOLBAR *: notification message structure
 *
 * @return LONG_PTR: must be a valid return value. See Win32 API, TBN_DROPDOWN
 */
LONG_PTR CMenuBar::Handle(const NMTOOLBAR *nmtb)
{
	if(nmtb->hdr.hwndFrom != m_hwndToolbar)
		return(TBDDRET_NODEFAULT);

	const int index = idToIndex(nmtb->iItem);
	invoke(nmtb->iItem);

	return(TBDDRET_DEFAULT);
}

/**
 * Invoke the dropdown menu for the button with the given control id.
 *
 * @param id     int: the control id of the toolbar button which has been activated
 */
void CMenuBar::invoke(const int id)
{
	const int index = idToIndex(id);

	HMENU	hMenu;

	m_isContactMenu = m_isMainMenu = false;

	_MessageWindowData *dat = (_MessageWindowData *)GetWindowLongPtr(m_pContainer->hwndActive, GWLP_USERDATA);

	HANDLE hContact = dat ? dat->hContact : 0;

	if(index == 3 && hContact != 0) {
		hMenu = reinterpret_cast<HMENU>(::CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM)hContact, 0));
		m_isContactMenu = true;
	} else if(index == 0) {
		hMenu = reinterpret_cast<HMENU>(::CallService(MS_CLIST_MENUBUILDMAIN, 0, 0));
		m_isMainMenu = true;
	} else
		hMenu = reinterpret_cast<HMENU>(m_TbButtons[index].dwData);

	RECT  rcButton;
	POINT pt;
	::SendMessage(m_hwndToolbar, TB_GETITEMRECT, (WPARAM)index, (LPARAM)&rcButton);
	pt.x = rcButton.left;
	pt.y = rcButton.bottom;
	::ClientToScreen(m_hwndToolbar, &pt);

	if(m_activeID)
		cancel(0);

	m_activeMenu = hMenu;
	m_activeSubMenu = 0;
	m_activeID = id;
	updateState(hMenu);
	obtainHook();
	m_fTracking = true;
	::SendMessage(m_hwndToolbar, TB_SETSTATE, (WPARAM)id, TBSTATE_CHECKED | TBSTATE_ENABLED);
	::TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, m_pContainer->hwnd, 0);
}

void CMenuBar::cancel(const int id)
{
	releaseHook();
	if(m_activeID)
		::SendMessage(m_hwndToolbar, TB_SETSTATE, (WPARAM)m_activeID, TBSTATE_ENABLED);
	m_activeID = 0;
	m_activeMenu = 0;
	m_isContactMenu = m_isMainMenu = false;
	::EndMenu();
}

/**
 * Cancel menu tracking completely
 */
void CMenuBar::Cancel(void)
{
	cancel(0);
	m_fTracking = false;
	autoShow(0);
}

void CMenuBar::updateState(const HMENU hMenu) const
{
	_MessageWindowData *dat = (_MessageWindowData *)GetWindowLongPtr(m_pContainer->hwndActive, GWLP_USERDATA);

	::CheckMenuItem(hMenu, ID_VIEW_SHOWMENUBAR, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_NOMENUBAR ? MF_UNCHECKED : MF_CHECKED);
	::CheckMenuItem(hMenu, ID_VIEW_SHOWSTATUSBAR, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_NOSTATUSBAR ? MF_UNCHECKED : MF_CHECKED);
	::CheckMenuItem(hMenu, ID_VIEW_SHOWAVATAR, MF_BYCOMMAND | dat->showPic ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_VIEW_SHOWTITLEBAR, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_NOTITLE ? MF_UNCHECKED : MF_CHECKED);

	::EnableMenuItem(hMenu, ID_VIEW_SHOWTITLEBAR, m_pContainer->bSkinned && CSkin::m_frameSkins ? MF_GRAYED : MF_ENABLED);

	::CheckMenuItem(hMenu, ID_VIEW_TABSATBOTTOM, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_TABSBOTTOM ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_VIEW_VERTICALMAXIMIZE, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_VERTICALMAX ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_TITLEBAR_USESTATICCONTAINERICON, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_STATICICON ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_VIEW_SHOWTOOLBAR, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_HIDETOOLBAR ? MF_UNCHECKED : MF_CHECKED);
	::CheckMenuItem(hMenu, ID_VIEW_BOTTOMTOOLBAR, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_BOTTOMTOOLBAR ? MF_CHECKED : MF_UNCHECKED);

	::CheckMenuItem(hMenu, ID_VIEW_SHOWMULTISENDCONTACTLIST, MF_BYCOMMAND | (dat->sendMode & SMODE_MULTIPLE) ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_VIEW_STAYONTOP, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_STICKY ? MF_CHECKED : MF_UNCHECKED);

	::CheckMenuItem(hMenu, ID_EVENTPOPUPS_DISABLEALLEVENTPOPUPS, MF_BYCOMMAND | m_pContainer->dwFlags & (CNT_DONTREPORT | CNT_DONTREPORTUNFOCUSED | CNT_ALWAYSREPORTINACTIVE) ? MF_UNCHECKED : MF_CHECKED);
	::CheckMenuItem(hMenu, ID_EVENTPOPUPS_SHOWPOPUPSIFWINDOWISMINIMIZED, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_DONTREPORT ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_EVENTPOPUPS_SHOWPOPUPSFORALLINACTIVESESSIONS, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_ALWAYSREPORTINACTIVE ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_EVENTPOPUPS_SHOWPOPUPSIFWINDOWISUNFOCUSED, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_DONTREPORTUNFOCUSED ? MF_CHECKED : MF_UNCHECKED);

	::CheckMenuItem(hMenu, ID_WINDOWFLASHING_USEDEFAULTVALUES, MF_BYCOMMAND | (m_pContainer->dwFlags & (CNT_NOFLASH | CNT_FLASHALWAYS)) ? MF_UNCHECKED : MF_CHECKED);
	::CheckMenuItem(hMenu, ID_WINDOWFLASHING_DISABLEFLASHING, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_NOFLASH ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_WINDOWFLASHING_FLASHUNTILFOCUSED, MF_BYCOMMAND | m_pContainer->dwFlags & CNT_FLASHALWAYS ? MF_CHECKED : MF_UNCHECKED);
}

/*
 * this updates the container menu bar and other window elements depending on the current child
 * session (IM, chat etc.). It fully supports IEView and will disable/enable the message log menus
 * depending on the configuration of IEView (e.g. when template mode is on, the message log settin
 * menus have no functionality, thus can be disabled to improve ui feedback quality).
 */

void CMenuBar::configureMenu() const
{
	_MessageWindowData *dat = (_MessageWindowData *)GetWindowLongPtr(m_pContainer->hwndActive, GWLP_USERDATA);

	BOOL fDisable = FALSE;

	if(dat) {
		bool fChat = (dat->bType == SESSIONTYPE_CHAT);

		::SendMessage(m_hwndToolbar, TB_SETSTATE, 103, fChat ? TBSTATE_HIDDEN : TBSTATE_ENABLED);
		::SendMessage(m_hwndToolbar, TB_SETSTATE, 104, fChat ? TBSTATE_ENABLED : TBSTATE_HIDDEN);
		::SendMessage(m_hwndToolbar, TB_SETSTATE, 105, fChat ? TBSTATE_HIDDEN : TBSTATE_ENABLED);

		if (dat->bType == SESSIONTYPE_IM)
			EnableWindow(GetDlgItem(dat->hwnd, IDC_TIME), fDisable ? FALSE : TRUE);
	}
}

/**
 * Automatically shows or hides the menu bar. Depends on the current state,
 * used when the ALT key is hit in the message window.
 */
void CMenuBar::autoShow(const int showcmd)
{
	if(m_mustAutoHide && !(m_pContainer->dwFlags & CNT_NOMENUBAR)) {
		m_pContainer->dwFlags |= CNT_NOMENUBAR;
		m_mustAutoHide = false;
		::SendMessage(m_pContainer->hwnd, WM_SIZE, 0, 1);
		releaseHook();
	}

	if(showcmd == 0) {
		::SetFocus(m_pContainer->hwndActive);
		return;
	}

	if(m_pContainer->dwFlags & CNT_NOMENUBAR) {
		m_mustAutoHide = true;
		m_pContainer->dwFlags &= ~CNT_NOMENUBAR;
		::SendMessage(m_pContainer->hwnd, WM_SIZE, 0, 1);
	}
	else											// do nothing, already visible
		m_mustAutoHide = false;
	//obtainHook();
	::SetFocus(m_hwndToolbar);
	//::SendMessage(m_hwndToolbar, TB_SETHOTITEM, 0, HICF_ACCELERATOR);
}

/**
 * Message hook function, installed by the menu handler to support
 * hot-tracking and keyboard navigation for the menu bar while a modal
 * popup menu is active.
 *
 * Hook is only active while a (modal) popup menu is processed.
 *
 * @params See Win32, message hooks
 *
 * @return
 */
LRESULT CALLBACK CMenuBar::MessageHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
        return(::CallNextHookEx(m_hHook, nCode,	wParam, lParam));

	MSG *pMsg = reinterpret_cast<MSG *>(lParam);

	if(nCode == MSGF_MENU) {
		switch(pMsg->message) {
			case WM_KEYDOWN:
				switch(pMsg->wParam) {
					case VK_ESCAPE: {
						int iIndex = m_Owner->idToIndex(m_Owner->m_activeID);
						if(iIndex != -1)
							::SendMessage(m_Owner->m_hwndToolbar, TB_SETHOTITEM, (WPARAM)iIndex, 0);
						::SetFocus(m_Owner->m_hwndToolbar);
						::SendMessage(m_Owner->m_hwndToolbar, TB_SETSTATE, (WPARAM)m_Owner->m_activeID, TBSTATE_ENABLED | TBSTATE_PRESSED);
						m_Owner->cancel(0);
						m_Owner->m_fTracking = false;
						break;
					}
					default:
						break;
				}
				break;

			case WM_NOTIFY: {
				break;;
			}

			case WM_MENUSELECT:
				break;

			case WM_LBUTTONDOWN: {
				POINT	pt;

				GetCursorPos(&pt);
				if(MenuItemFromPoint(0, m_Owner->m_activeMenu, pt) >= 0) 			// inside menu
					break;
				if(m_Owner->m_activeSubMenu && MenuItemFromPoint(0, m_Owner->m_activeSubMenu, pt) >= 0)
					break;
				else {																// anywhere else, cancel the menu
					::CallNextHookEx(m_hHook, nCode, wParam, lParam);
					m_Owner->Cancel();
					return(0);
				}
			}
			/*
			 * allow hottracking by the toolbar control
			 */
			case WM_MOUSEMOVE: {
				POINT pt;

				GetCursorPos(&pt);
				ScreenToClient(m_Owner->m_hwndToolbar, &pt);
				LPARAM newPos = MAKELONG(pt.x, pt.y);
				::SendMessage(m_Owner->m_hwndToolbar, pMsg->message, pMsg->wParam, newPos);
				break;
			}
			default:
				break;
		}
	}
	return(::CallNextHookEx(m_hHook, nCode, wParam, lParam));
}

/*
 *  window procedure for the status bar class.
 */

static 	int 	tooltip_active = FALSE;
static 	POINT 	ptMouse = {0};
RECT 	rcLastStatusBarClick;						// remembers click (down event) point for status bar clicks

LONG_PTR CALLBACK StatusBarSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ContainerWindowData *pContainer = (ContainerWindowData *)GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);

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
			if ((pContainer && pContainer->bSkinned) || M->isAero())
				return 1;
			break;
		}
		case WM_PAINT: {
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
			else if(fAero)
				CSkin::ApplyAeroEffect(hdcMem, &rcClient, CSkin::AERO_EFFECT_AREA_STATUSBAR);
			else
				FillRect(hdcMem, &rcClient, GetSysColorBrush(COLOR_3DFACE));

			for (i = 0; i < (int)nParts; i++) {
				SendMessage(hWnd, SB_GETRECT, (WPARAM)i, (LPARAM)&itemRect);
				if (!item->IGNORED && !fAero && pContainer && pContainer->bSkinned)
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

			if (pContainer->dwFlags & CNT_NOTITLE) {
				POINT	pt1 = pt;
				RECT	rcIconpart;

				ScreenToClient(hWnd, &pt1);
				SendMessage(hWnd, SB_GETRECT, 2, (LPARAM)&rcIconpart);
				if(!PtInRect(&rcIconpart, pt1))
					return SendMessage(pContainer->hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(pt.x, pt.y));
			}
			break;
		}

		case WM_TIMER:
			if (wParam == TIMERID_HOVER) {
				POINT pt;
#if defined(_UNICODE)
				char *szTTService = "mToolTip/ShowTipW";
#else
				char *szTTService = "mToolTip/ShowTip";
#endif
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
						StatusIconListNode *current = status_icon_list;
						StatusIconListNode *clicked = NULL;
						StatusIconListNode *currentSIN = NULL;

						unsigned int iconNum = (pt.x - rc.left) / (PluginConfig.m_smcxicon + gap);
						unsigned int list_icons = 0;
						char         buff[100];
						DWORD        flags;

						while (current) {
							if(current->sid.flags&MBF_OWNERSTATE&&dat->pSINod){
								StatusIconListNode *currentSIN = dat->pSINod;
								flags=current->sid.flags;
								while (currentSIN)	{
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

 						if(clicked&&clicked->sid.flags&MBF_OWNERSTATE) {
 							currentSIN=dat->pSINod;
 							while (currentSIN) {
 								if (strcmp(currentSIN->sid.szModule, clicked->sid.szModule) == 0 && currentSIN->sid.dwId == clicked->sid.dwId) {
 									clicked=currentSIN;
 									break;
 								}
 								currentSIN = currentSIN->next;
 							}
 						}

						if ((int)iconNum == list_icons && pContainer) {
							TCHAR wBuf[512];

							mir_sntprintf(wBuf, safe_sizeof(wBuf), TranslateT("Sounds are %s. Click to toggle status, hold SHIFT and click to set for all open containers"), pContainer->dwFlags & CNT_NOSOUND ? TranslateT("disabled") : TranslateT("enabled"));
							CallService(szTTService, (WPARAM)wBuf, (LPARAM)&ti);
							tooltip_active = TRUE;
						}
						else if ((int)iconNum == list_icons + 1 && dat && dat->bType == SESSIONTYPE_IM) {
							int mtnStatus = (int)M->GetByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, M->GetByte(SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW));
							TCHAR wBuf[512];

							mir_sntprintf(wBuf, safe_sizeof(wBuf), TranslateT("Sending typing notifications is %s."), mtnStatus ? TranslateT("enabled") : TranslateT("disabled"));
							CallService(szTTService, (WPARAM)wBuf, (LPARAM)&ti);
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
						GETTEXTLENGTHEX gtxl = {0};

						gtxl.codepage = CP_UTF8;
						gtxl.flags = GTL_DEFAULT | GTL_PRECISE | GTL_NUMBYTES;
						iLength = SendDlgItemMessage(dat->hwnd, dat->bType == SESSIONTYPE_IM ? IDC_MESSAGE : IDC_CHAT_MESSAGE, EM_GETTEXTLENGTHEX, (WPARAM) & gtxl, 0);
						tooltip_active = TRUE;

						TCHAR wBuf[512];
						TCHAR *szFormat = _T("There are %d pending send jobs. Message length: %d bytes, message length limit: %d bytes");

						mir_sntprintf(wBuf, safe_sizeof(wBuf), TranslateTS(szFormat), dat->iOpenJobs, iLength, dat->nMax ? dat->nMax : 20000);
						CallService(szTTService, (WPARAM)wBuf, (LPARAM)&ti);
					}
					//MAD
					if(SendMessage(dat->pContainer->hwndStatus, SB_GETTEXT, 0, (LPARAM)szStatusBarText)) {
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
							CallService(szTTService, (WPARAM)dbv.ptszVal, (LPARAM)&ti);
							if(dbv.pszVal)
								DBFreeVariant(&dbv);
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

