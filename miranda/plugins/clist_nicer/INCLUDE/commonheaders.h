/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2010 Miranda ICQ/IM project,
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
 * part of clist_nicer plugin for Miranda.
 *
 * (C) 2005-2010 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id: commonheaders.h 12813 2010-09-26 11:09:45Z borkra $
 *
 */

#define MIRANDA_VER 0x0700

#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0501

#define  EXTRA_ICON_COUNT	11

#undef FASTCALL

#define TSAPI __stdcall
#define FASTCALL __fastcall

#include <m_stdhdr.h>
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include <io.h>
#include <string.h>
#include <direct.h>
#include <math.h>
#include <shlwapi.h>
#include "resource.h"
#include <newpluginapi.h>
#include <win2k.h>
#include <m_clist.h>
#include <m_clistint.h>
#include <m_clc.h>
#include <m_clui.h>
#include <m_plugins.h>
#include <m_system.h>
#include <m_utils.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_button.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <m_skin.h>
#include <m_contacts.h>
#include <m_file.h>
#include <m_addcontact.h>
#include <m_message.h>
#include <m_timezones.h>
#include <m_genmenu.h>
#include <m_cluiframes.h>
#include <m_clui.h>
#include <m_icolib.h>
#include <m_popup.h>
#include <m_fontservice.h>
#include <m_metacontacts.h>
#include <m_cln_skinedit.h>

#include "m_avatars.h"
#include "extbackg.h"
#include "clc.h"
#include <config.h>


#include "clist.h"
#include "alphablend.h"
#include "rowheight_funcs.h"

	/*
	 * text shadow types (DrawThemeTextEx() / Vista+ uxtheme)
	 */
	#define TST_NONE			0
	#define TST_SINGLE			1
	#define TST_CONTINUOUS		2

	typedef struct _DWM_THUMBNAIL_PROPERTIES
	{
		DWORD dwFlags;
		RECT rcDestination;
		RECT rcSource;
		BYTE opacity;
		BOOL fVisible;
		BOOL fSourceClientAreaOnly;
	} DWM_THUMBNAIL_PROPERTIES, *PDWM_THUMBNAIL_PROPERTIES;

	enum DWMWINDOWATTRIBUTE
	{
	    DWMWA_NCRENDERING_ENABLED = 1,      // [get] Is non-client rendering enabled/disabled
	    DWMWA_NCRENDERING_POLICY,           // [set] Non-client rendering policy
	    DWMWA_TRANSITIONS_FORCEDISABLED,    // [set] Potentially enable/forcibly disable transitions
	    DWMWA_ALLOW_NCPAINT,                // [set] Allow contents rendered in the non-client area to be visible on the DWM-drawn frame.
	    DWMWA_CAPTION_BUTTON_BOUNDS,        // [get] Bounds of the caption button area in window-relative space.
	    DWMWA_NONCLIENT_RTL_LAYOUT,         // [set] Is non-client content RTL mirrored
	    DWMWA_FORCE_ICONIC_REPRESENTATION,  // [set] Force this window to display iconic thumbnails.
	    DWMWA_FLIP3D_POLICY,                // [set] Designates how Flip3D will treat the window.
	    DWMWA_EXTENDED_FRAME_BOUNDS,        // [get] Gets the extended frame bounds rectangle in screen space
	    DWMWA_HAS_ICONIC_BITMAP,            // [set] Indicates an available bitmap when there is no better thumbnail representation.
	    DWMWA_DISALLOW_PEEK,                // [set] Don't invoke Peek on the window.
	    DWMWA_EXCLUDED_FROM_PEEK,           // [set] LivePreview exclusion information
	    DWMWA_LAST
	};

	#define DWM_TNP_RECTDESTINATION	0x00000001
	#define DWM_TNP_RECTSOURCE 0x00000002
	#define DWM_TNP_OPACITY	0x00000004
	#define DWM_TNP_VISIBLE	0x00000008
	#define DWM_TNP_SOURCECLIENTAREAONLY 0x00000010

	#define DWM_SIT_DISPLAYFRAME    0x00000001  // Display a window frame around the provided bitmap

	typedef HANDLE HTHUMBNAIL;
	typedef HTHUMBNAIL* PHTHUMBNAIL;

#ifndef BPPF_ERASE
	typedef enum _BP_BUFFERFORMAT
	{
		BPBF_COMPATIBLEBITMAP,    // Compatible bitmap
		BPBF_DIB,                 // Device-independent bitmap
		BPBF_TOPDOWNDIB,          // Top-down device-independent bitmap
		BPBF_TOPDOWNMONODIB       // Top-down monochrome device-independent bitmap
	} BP_BUFFERFORMAT;


	typedef struct _BP_PAINTPARAMS
	{
		DWORD                       cbSize;
		DWORD                       dwFlags; // BPPF_ flags
		const RECT *                prcExclude;
		const BLENDFUNCTION *       pBlendFunction;
	} BP_PAINTPARAMS, *PBP_PAINTPARAMS;

	#define BPPF_ERASE               1
	#define BPPF_NOCLIP              2
	#define BPPF_NONCLIENT           4
#endif

	typedef struct _DWM_BLURBEHIND
	{
		DWORD dwFlags;
		BOOL fEnable;
		HRGN hRgnBlur;
		BOOL fTransitionOnMaximized;
	} DWM_BLURBEHIND, *PDWM_BLURBEHIND;

	#define DWM_BB_ENABLE 1

#ifndef LOCALE_SISO3166CTRYNAME2
	#define LOCALE_SISO3166CTRYNAME2      0x00000068   // 3 character ISO country name, eg "USA Vista+
	#define LOCALE_SISO639LANGNAME2       0x00000067   // 3 character ISO abbreviated language name, eg "eng"
#endif

#ifndef WM_DWMCOMPOSITIONCHANGED
	#define WM_DWMCOMPOSITIONCHANGED        0x031E
	#define WM_DWMCOLORIZATIONCOLORCHANGED  0x0320
#endif

#ifndef WM_DWMSENDICONICTHUMBNAIL
	#define WM_DWMSENDICONICTHUMBNAIL           0x0323
	#define WM_DWMSENDICONICLIVEPREVIEWBITMAP   0x0326
#endif

// shared vars
extern HINSTANCE g_hInst;

/* most free()'s are invalid when the code is executed from a dll, so this changes
 all the bad free()'s to good ones, however it's still incorrect code. The reasons for not
 changing them include:

  * DBFreeVariant has a CallService() lookup
  * free() is executed in some large loops to do with clist creation of group data
  * easy search and replace

*/

#define MAX_REGS(_A_) (sizeof(_A_)/sizeof(_A_[0]))

extern struct LIST_INTERFACE li;
typedef  int  (__cdecl *pfnDrawAvatar)(HDC hdcOrig, HDC hdcMem, RECT *rc, struct ClcContact *contact, int y, struct ClcData *dat, int selected, WORD cstatus, int rowHeight);

#define safe_sizeof(a) (sizeof((a)) / sizeof((a)[0]))

BOOL __forceinline GetItemByStatus(int status, StatusItems_t *retitem);

void DrawAlpha(HDC hdcwnd, PRECT rc, DWORD basecolor, int alpha, DWORD basecolor2, BOOL transparent, BYTE FLG_GRADIENT, BYTE FLG_CORNER, DWORD BORDERSTYLE, ImageItem *item);

void FreeAndNil( void** );

#if _MSC_VER >= 1500
	#define wEffects wReserved
#endif

