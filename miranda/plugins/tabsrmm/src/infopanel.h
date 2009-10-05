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
 * the info area for both im and chat sessions
 */

#ifndef __INFOPANEL_H
#define __INFOPANEL_H

/*
 * configuration data for the info panel (fonts, colors)
 * this is global for all panels
 */

#define IPFONTCOUNT 6						// # of fonts needed for the info panel

/*
 * command ids for the info panel contextual menus
 */

#define CMD_IP_USERDETAILS					20000
#define CMD_IP_USERPREFS					20001
#define CMD_IP_ROOMPREFS					20002

struct InfoPanelConfig {
	HFONT       hFonts[IPFONTCOUNT];
	COLORREF    clrs[IPFONTCOUNT];
	COLORREF    clrClockSymbol, clrBackground;
	HBRUSH      bkgBrush;
	UINT		height1, height2;
};

extern TCHAR *xStatusDescr[];

class CInfoPanel
{
public:
	enum {
		DEGRADE_THRESHOLD = 37,					// defines the height at which the infopanel will do the transition from 1 to 2 lines
		LEFT_OFFSET_LOGO = 3
	};
	enum {
		HOVER_NICK 		= 1,
		HOVER_STATUS 	= 2,
		HOVER_UIN 		= 4
	};

	CInfoPanel(_MessageWindowData *dat)
	{
		if(dat) {
			m_dat = dat;
			m_isChat = dat->bType == SESSIONTYPE_CHAT ? true : false;
		}
		m_defaultHeight = PluginConfig.m_panelHeight;
		m_defaultMUCHeight = PluginConfig.m_MUCpanelHeight;
		m_hwndConfig = 0;
		m_hoverFlags = 0;
	}

	~CInfoPanel()
	{
		if(m_hwndConfig)
			::DestroyWindow(m_hwndConfig);
		saveHeight(true);
	}

	const	LONG				getHeight					() const { return(m_height); }
	void						setHeight					(LONG newHeight, bool fBroadcast = false);
	bool						isActive					() const { return(m_active); }
	bool						isPrivateHeight				() const { return(m_fPrivateHeight); }
	DWORD						isHovered					() const { return(m_active ? m_hoverFlags : 0); }
	void						setActive					(const int newActive);
	void						loadHeight					();
	void						saveHeight					(bool fFlush = false);

	void 						Configure					() const;
	void 						showHide					() const;
	bool 						getVisibility				();
	void 						renderBG					(const HDC hdc, RECT& rc, CSkinItem *item, bool fAero) const;
	void 						renderContent				(const HDC hdcMem);
	void 						Invalidate					() const;
	void 						trackMouse					(POINT& pt);
	void						handleClick					(const POINT& pt);
	void						showTip						(UINT ctrlId, const LPARAM lParam) const;
	int							invokeConfigDialog			(const POINT& pt);
	void						dismissConfig				(bool fForced = false)
	{
		if(m_hwndConfig == 0)
			return;

		POINT pt;
		RECT  rc;

		::GetCursorPos(&pt);
		::GetWindowRect(m_hwndConfig, &rc);
		if(fForced || !PtInRect(&rc, pt)) {
			SendMessage(m_hwndConfig, WM_CLOSE, 1, 1);
			m_hwndConfig = 0;
		}
	}
public:
	static						InfoPanelConfig m_ipConfig;

private:
	void						mapRealRect					(const RECT& rcSrc, RECT& rcDest, const SIZE& sz);
	HFONT						setUnderlinedFont			(const HDC hdc, HFONT hFontOrig);
	void						RenderIPNickname			(const HDC hdc, RECT& rc);
	void 						RenderIPUIN					(const HDC hdc, RECT& rcItem);
	void 						RenderIPStatus				(const HDC hdc, RECT& rcItem);
	void 						Chat_RenderIPNickname		(const HDC hdc, RECT& rcItem);
	void 						Chat_RenderIPSecondLine		(const HDC hdc, RECT& rcItem);
	HMENU						constructContextualMenu		() const;
	INT_PTR	CALLBACK			ConfigDlgProc				(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK	 	ConfigDlgProcStub			(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	bool				m_isChat;											// is MUC session
	bool				m_active;											// panel active and visible
	bool				m_fPrivateHeight;
	_MessageWindowData*	m_dat;												// this one OWNS us...
	LONG				m_height;											// height (determined by position of IDC_PANELSPLITTER)
	LONG				m_defaultHeight, m_defaultMUCHeight;				// global values for the info bar height
	HWND				m_hwndConfig;										// window handle of the config dialog window
	HFONT				m_configDlgFont, m_configDlgBoldFont;
	SIZE				m_szNick;											// rectangle where the nick has been rendered,
																			// used for hovering and showing tooltips
	/*
	 * these are used to store rectangles important to mouse tracking.
	 */
	RECT				m_rcNick;
	RECT				m_rcUIN;
	RECT				m_rcStatus;
	DWORD				m_hoverFlags;
};

#endif /* __INFOPANEL_H */

