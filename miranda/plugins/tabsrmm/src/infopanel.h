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
 */

struct InfoPanelConfig {
	HFONT       hFonts[IPFONTCOUNT];
	COLORREF    clrs[IPFONTCOUNT];
	COLORREF    clrClockSymbol, clrBackground;
	BOOL        isValid;                   // valid data exist (font service required, otherwise, defaults are used)
	HBRUSH      bkgBrush;
	UINT		height1, height2;
};

extern TCHAR *xStatusDescr[];

class CInfoPanel
{
public:
	enum {
		DEGRADE_THRESHOLD = 39,					// defines the height at which the infopanel will transition from 1 to 2 lines
		LEFT_OFFSET_LOGO = 3
	};
	CInfoPanel(_MessageWindowData *dat)
	{
		if(dat) {
			m_dat = dat;
			m_isChat = dat->bType == SESSIONTYPE_CHAT ? true : false;
		}
		m_defaultHeight = PluginConfig.m_panelHeight;
		m_defaultMUCHeight = PluginConfig.m_MUCpanelHeight;
	}

	~CInfoPanel()
	{}

	const	LONG	getHeight() const { return(m_height); }
	void			setHeight(LONG newHeight) { m_height = newHeight; }
	bool			isActive() const { return(m_active); }
	void			setActive(const int newActive);
	void			loadHeight();
	void			saveHeight();

	void 			Configure() const;
	void 			showHideControls(const UINT showCmd) const;
	void 			showHide() const;
	bool 			getVisibility();
	void 			renderBG(const HDC hdc, RECT& rc, CSkinItem *item, bool fAero) const;
	void 			renderContent(const HDC hdcMem);
	void 			Invalidate() const;
	void 			trackMouse(POINT& pt) const;
	void			showTip(UINT ctrlId, const LPARAM lParam) const;

public:
	static			InfoPanelConfig m_ipConfig;

private:
	void			RenderIPNickname(const HDC hdc, RECT& rc);
	void 			RenderIPUIN(const HDC hdc, RECT& rcItem);
	void 			RenderIPStatus(const HDC hdc, RECT& rcItem);
	void 			Chat_RenderIPNickname(const HDC hdc, RECT& rcItem);
	void 			Chat_RenderIPSecondLine(const HDC hdc, RECT& rcItem);

private:
	bool				m_isChat;											// is MUC session
	bool				m_active;											// panel active and visible
	_MessageWindowData*	m_dat;												// this one OWNS us...
	LONG				m_height;											// height (determined by position of IDC_PANELSPLITTER)
	LONG				m_defaultHeight, m_defaultMUCHeight;				// global values for the info bar height
};

#endif /* __INFOPANEL_H */

