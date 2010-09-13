/*
Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2010 Miranda ICQ/IM project,
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

#ifndef __M_TIMEZONES_H
#define __M_TIMEZONES_H

#define MIM_TZ_NAMELEN 64

#define TZF_PLF_CB		1				// UI element is assumed to be a combo box
#define TZF_PLF_LB		2				// UI element is assumed to be a list box
#define TZF_HCONTACT    4
#define TZF_UNICODE	    8
#define TZF_DIFONLY    16
#define TZF_KNOWNONLY  32

#ifdef _UNICODE
#define TZF_TCHAR TZF_UNICODE
#else
#define TZF_TCHAR 0
#endif

typedef struct _tagPrepareList {
	DWORD	cbSize;							// caller must supply this
	HWND	hWnd;							// window handle of the combo or list box
	TCHAR	tszName[MIM_TZ_NAMELEN];		// tz name (for preselecting)
	DWORD	dwFlags;						// flags - if neither PLF_CB or PLF_LB is set, the window class name will be used
											// to figure out the type of control.
	HANDLE	hContact;						// contact handle (for preselecting)
											// the contact handle has precendence over tszName[].
											// data will be read from the database (if present). Otherwise, the
											// <unspecified> entry will be preselected.
} MIM_TZ_PREPARELIST;


/*
 * Obtain time zone information by time zone name
 * wParam = (TCHAR *)tszName
 * lParam =  dwFlags
 *
 * returns pointer to TZHANDLE if everything ok, 0 otherwise.
 */
#define MS_TZ_GETINFOBYNAME  "TZ2/GetInfoByName"


/*
 * Obtain time zone information by contact handle
 * wParam = (HANDLE)hContact
 * lParam =  dwFlags
 *
 * returns TZHANDLE if everything ok, 0 otherwise.
 */
#define MS_TZ_GETINFOBYCONTACT "TZ2/GetInfoByContact"


/*
 * Set time zone information by contact handle
 * wParam = (HANDLE)hContact - Contact handle
 * lParam =  TZHANDLE - Time zone Handle
 *
 * returns 0
 */
#define MS_TZ_SETINFOBYCONTACT "TZ2/SetInfoByContact"

typedef struct 
{
	int cbSize;
	HANDLE hTimeZone;
	const TCHAR *szFormat;  // format string, as above
	TCHAR *szDest;          // place to put the output string
	int cbDest;             // maximum number of bytes to put in szDest
	int flags;
} TZTOSTRING;

#define MS_TZ_PRINTDATETIME "TZ2/PrintDateTime"

/*
 * this service fills a combo or list box with time zone information
 * optionally, it can preselect a value based on the tszName[] and hContact members.
 * the name member has precendence over the contact handle. If neither one is present,
 * no preselection will be performed.
 *
 * wParam = 0 (not used)
 * lParam = MIM_TZ_PREPARELIST *
 *
 * returns: 0 on failure, != otherwise
 *
 * How to obtain the entry selected by the user?
 *
 * 1) send CB_GETCURSEL / LB_GETCURSEL to the list/combo box to obtain the selection
 * 2) use CB_GETITEMDATA / LB_GETITEMDATA with the selection to get a pointer to a
 * 	  MIM_TIMEZONE structure.  This pointer CAN be NULL (when the user did not
 * 	  select a valid time zone, so check it.)
 *
 * 	  DO NOT modify the data inside the struct. It's supposed to be read-only.
 *
 */
#define MS_TZ_PREPARELIST "TZ2/PrepareList"

/*
 * Store selected listbox setting in the database
 * wParam = (HANDLE)hContact - Contact handle
 * lParam =  hWnd - Listbox/ComboBox window handle
 *
 * returns 0
 */
#define MS_TZ_STORELISTRESULT "TZ2/StoreListResult"

#endif /* __M_TIMEZONES_H */
