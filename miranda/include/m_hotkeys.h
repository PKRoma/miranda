/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project, 
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
*/

#ifndef M_HOTKEYS_H__
#define M_HOTKEYS_H__ 1

typedef struct
{
	int cbSize;
	const char *pszName;			/* name to refer to hotkey when playing and in db */
	const char *pszDescription;		/* description for options dialog */
    const char *pszSection;			/* section name used to group sounds (NULL is acceptable) */
	const char *pszService;			/* Service to call when HotKey Pressed */
	WORD DefHotKey;					/* default hot key for action */
	LPARAM lParam;					/* lParam to pass to service */
} HOTKEYDESC;

#define HKF_MIRANDA_LOCAL		0x8000

/* use this macro to defile hotkey codes like this:
  hkd.DefHotkey = HOTKEYCODE(HOTKEYF_SHIFT|HOTKEYF_EXT, 'A');
*/
#define HOTKEYCODE(mod,vk)			(MAKEWORD((vk),(mod)))

/* CoreHotkeys/Register service
Registers new hotkey
  wParam=(WPARAM)0
  lParam=(LPARAM)(HOTKEYDESC *)hotkey
Returns 0 on success or nonzero otherwise
*/
#define MS_HOTKEY_REGISTER			"CoreHotkeys/Register"

/* CoreHotkeys/Register service
Registers new hotkey
  wParam=(WPARAM)0
  lParam=(LPARAM)(char *)pszName
Returns 0 on success or nonzero otherwise
*/
#define MS_HOTKEY_UNREGISTER		"CoreHotkeys/Unregister"

/* CoreHotkeys/Register service
Checks if "manual" hotkey was activated and returns its id.
  wParam=(WPARAM)(MSG *)message
  lParam=(LPARAM)(char *)pszSection
Returns lParam associated with activated hotkey
*/
#define MS_HOTKEY_CHECK				"CoreHotkeys/Check"

/* Subclass/unsubclass edit box to act as hotkey control
  wParam=(WPARAM)(HWND)hwndEdit
  lParam=(LPARAM)0
Returns zero on success

You will get notification with LOWORD(wParam) == 0 when users sets hotkey.

Subclassed control processes HKM_SETHOTKEY and HKM_GETHOTKEY similarly to
windows' hotkey, in all other it acts just like normal editbox (beware of
standart notifications that occur on text updates!)

Subclass procedure uses GWL_USERDATA to store internal information. Do not
use it for own purposes.
*/
#define MS_HOTKEY_SUBCLASS			"CoreHotkeys/Subclass"
#define MS_HOTKEY_UNSUBCLASS		"CoreHotkeys/Unsubclass"

#endif // M_HOTKEYS_H__





