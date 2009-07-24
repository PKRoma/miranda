/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
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

$id$

*/

#include "commonheaders.h"

/*
 * read a setting for a contact
 */

DWORD _Mim::GetDword(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, DWORD uDefault = 0) const
{
	return((DWORD)DBGetContactSettingDword(hContact, szModule, szSetting, uDefault));
}

/*
 * read a setting from our default module (Tab_SRMSG)
 */

DWORD _Mim::GetDword(const char *szSetting = 0, DWORD uDefault = 0) const
{
	return((DWORD)DBGetContactSettingDword(0, SRMSGMOD_T, szSetting, uDefault));
}

/*
 * read a contact setting with our default module name (Tab_SRMSG)
 */

DWORD _Mim::GetDword(const HANDLE hContact = 0, const char *szSetting = 0, DWORD uDefault = 0) const
{
	return((DWORD)DBGetContactSettingDword(hContact, SRMSGMOD_T, szSetting, uDefault));
}

/*
 * read a setting from module only
 */

DWORD _Mim::GetDword(const char *szModule, const char *szSetting, DWORD uDefault) const
{
	return((DWORD)DBGetContactSettingDword(0, szModule, szSetting, uDefault));
}

/*
 * same for bytes now
 */
int _Mim::GetByte(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, int uDefault = 0) const
{
	return(DBGetContactSettingByte(hContact, szModule, szSetting, uDefault));
}

int _Mim::GetByte(const char *szSetting = 0, int uDefault = 0) const
{
	return(DBGetContactSettingByte(0, SRMSGMOD_T, szSetting, uDefault));
}

int _Mim::GetByte(const HANDLE hContact = 0, const char *szSetting = 0, int uDefault = 0) const
{
	return(DBGetContactSettingByte(hContact, SRMSGMOD_T, szSetting, uDefault));
}

int _Mim::GetByte(const char *szModule, const char *szSetting, int uDefault) const
{
	return(DBGetContactSettingByte(0, szModule, szSetting, uDefault));
}

/*
 * writer functions
 */

INT_PTR _Mim::WriteDword(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, DWORD value = 0) const
{
	return(DBWriteContactSettingDword(hContact, szModule, szSetting, value));
}

/*
 * write non-contact setting
*/

INT_PTR _Mim::WriteDword(const char *szModule = 0, const char *szSetting = 0, DWORD value = 0) const
{
	return(DBWriteContactSettingDword(0, szModule, szSetting, value));
}

INT_PTR _Mim::WriteByte(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, BYTE value = 0) const
{
	return(DBWriteContactSettingByte(hContact, szModule, szSetting, value));
}

INT_PTR _Mim::WriteByte(const char *szModule = 0, const char *szSetting = 0, BYTE value = 0) const
{
	return(DBWriteContactSettingByte(0, szModule, szSetting, value));
}

/*
 * window list functions
 */

void _Globals::BroadcastMessage(UINT msg = 0, WPARAM wParam = 0, LPARAM lParam = 0)
{
	WindowList_Broadcast(m_hMessageWindowList, msg, wParam, lParam);
}

void _Globals::BroadcastMessageAsync(UINT msg = 0, WPARAM wParam = 0, LPARAM lParam = 0)
{
	WindowList_BroadcastAsync(m_hMessageWindowList, msg, wParam, lParam);
}

HWND _Globals::FindWindow(HANDLE h = 0) const
{
	return(WindowList_Find(m_hMessageWindowList, h));
}

INT_PTR _Globals::AddWindow(HWND hWnd = 0, HANDLE h = 0)
{
	return(WindowList_Add(m_hMessageWindowList, hWnd, h));
}

INT_PTR _Globals::RemoveWindow(HWND hWnd = 0)
{
	return(WindowList_Remove(m_hMessageWindowList, hWnd));
}

_Globals Globals;
_Globals *pGlobals = &Globals;

_Mim Mim;
_Mim *pMim = &Mim;

