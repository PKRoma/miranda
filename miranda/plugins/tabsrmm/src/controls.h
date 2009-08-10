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
 * menu bar and status bar classes for the container window.
 *
 */

#ifndef __CONTROLS_H
#define __CONTROLS_H

extern LONG_PTR CALLBACK StatusBarSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class CMenuBar
{
public:
	CMenuBar(HWND hwndParent, const ContainerWindowData *pContainer);
	~CMenuBar();

	const RECT&		getClientRect();
	void			Resize(WORD wWidth, WORD wHeight) const
	{
		::MoveWindow(m_hwndRebar, 0, 0, wWidth, wHeight, FALSE);
	}
	LONG			getHeight() const;
	void			Show(int showCmd) const
	{
		::ShowWindow(m_hwndRebar, showCmd);
	}
private:
	HWND		m_hwndRebar;
	HWND		m_hwndToolbar;
	RECT		m_rcClient;
	ContainerWindowData *m_pContainer;
};

#endif /* __CONTROLS_H */
