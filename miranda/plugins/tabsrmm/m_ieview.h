/*

IEView Plugin for Miranda IM
Copyright (C) 2005  Piotr Piastucki

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

#ifndef M_IEVIEW_INCLUDED
#define M_IEVIEW_INCLUDED

#define MS_IEVIEW_WINDOW  "IEVIEW/NewWindow"
#define MS_IEVIEW_EVENT	  "IEVIEW/Event"
#define MS_IEVIEW_SHOWSMILEYSELECTION  "IEVIEW/ShowSmileySelection"

#define IEW_CREATE  1               // create new window (control)
#define IEW_DESTROY 2               // destroy control
#define IEW_SETPOS  3               // set window position and size

#define IEWM_SRMM     0             // regular SRMM
#define IEWM_TABSRMM  1             // TabSRMM-compatible HTML builder
#define IEWM_HTML     2             // HTML - unsupported at the moment

typedef struct {
	int			cbSize;             // size of the strusture
	int			iType;				// one of IEW_* values
	DWORD		dwMode;				// compatibility mode - one of IEWM_* values
	DWORD		dwFlags;			// flags, one of IEWF_* values
	HWND		parent;             // parent window HWND
	HWND 		hwnd;               // IEW_CREATE returns WebBrowser control's HWND here
	int			x;                  // IE control horizontal position
	int			y;                  // IE control vertical position
	int			cx;                 // IE control horizontal size
	int			cy;                 // IE control vertical size
} IEVIEWWINDOW;

#define IEE_LOG_EVENTS  1               // log specified number of DB events
#define IEE_CLEAR_LOG	2               // clear log

typedef struct {
	int			cbSize;             // size of the strusture
	int			iType;				// one of IEE_* values
	DWORD		dwFlags;			// one of IEEF_* values
	HWND		hwnd;               // HWND returned by IEW_CREATE
	HANDLE      hContact;           // contact
	HANDLE 		hDbEventFirst;      // first event to log
	int 		count;              // number of events to log
} IEVIEWEVENT;

#endif

