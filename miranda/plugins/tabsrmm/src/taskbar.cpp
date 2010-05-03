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
 *
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
		DWORD	size = 4;
		DWORD	dwType = REG_DWORD;
		/*
		 * check whether the taskbar is set to show large icons. This is necessary, because the method SetOverlayIcon()
		 * always returns S_OK, but the icon is simply ignored when using small taskbar icons.
		 */
		if(::RegOpenKey(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), &hKey) == ERROR_SUCCESS) {
			::RegQueryValueEx(hKey, _T("TaskbarSmallIcons"), 0, &dwType, (LPBYTE)&val, &size);
			::RegCloseKey(hKey);
		}
		m_fHaveLargeicons = (val ? false : true);			// small icons in use, revert to default icon feedback
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
