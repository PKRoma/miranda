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
 * Windows 7 taskbar integration
 */

#ifndef __TASKBAR_H
#define __TASKBAR_H

class CTaskbarInteract
{
public:
	CTaskbarInteract()
	{
		m_pTaskbarInterface = 0;
		m_IconSize = 0;
		m_isEnabled = (IsWinVer7Plus() && M->GetByte("useW7Taskbar", 1));

		if(m_isEnabled) {
			::CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_ALL, IID_ITaskbarList3, (void**)&m_pTaskbarInterface);
			updateMetrics();
		}
	}

	~CTaskbarInteract()
	{
		if(m_isEnabled && m_pTaskbarInterface) {
			m_pTaskbarInterface->Release();
			m_pTaskbarInterface = 0;
			m_isEnabled = false;
		}
	}
	LONG			getIconSize						() const { return(m_IconSize); }

	bool 			setOverlayIcon					(HWND hwndDlg, LPARAM lParam) const;
	void			clearOverlayIcon				(HWND hwndDlg) const;
	bool			haveLargeIcons					();
	LONG			updateMetrics					();

private:
	bool 										m_isEnabled;
	ITaskbarList3* 								m_pTaskbarInterface;
	bool										m_fHaveLargeicons;
	LONG										m_IconSize;
};

extern CTaskbarInteract* Win7Taskbar;

#endif /* __TASKBAR_H */

