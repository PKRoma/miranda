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
 * global include file, used to build the precompiled header.
 *
 */


#ifndef __COMMONHEADERS_H
#define __COMMONHEADERS_H

#if defined( UNICODE ) && !defined( _UNICODE )
	#define _UNICODE
#endif

#define WINVER 0x0600
#if _MSC_VER >= 1500
	#define _WIN32_WINNT 0x0600
#else
	#define _WIN32_WINNT 0x0501
#endif
#define _WIN32_IE 0x0501

#define MIRANDA_VER 0x0700

#include <m_stdhdr.h>

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#if _MSC_VER >= 1500
	#include <dwmapi.h>
#else
	typedef struct _DWM_THUMBNAIL_PROPERTIES
	{
		DWORD dwFlags;
		RECT rcDestination;
		RECT rcSource;
		BYTE opacity;
		BOOL fVisible;
		BOOL fSourceClientAreaOnly;
	} DWM_THUMBNAIL_PROPERTIES, *PDWM_THUMBNAIL_PROPERTIES;

	typedef HANDLE HTHUMBNAIL;
	typedef HTHUMBNAIL* PHTHUMBNAIL;

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

	typedef struct _DWM_BLURBEHIND
	{
		DWORD dwFlags;
		BOOL fEnable;
		HRGN hRgnBlur;
		BOOL fTransitionOnMaximized;
	} DWM_BLURBEHIND, *PDWM_BLURBEHIND;

	#define BPPF_ERASE               1
	#define BPPF_NOCLIP              2
	#define BPPF_NONCLIENT           4

	#define DWM_BB_ENABLE 1

#endif
#define WM_DWMCOMPOSITIONCHANGED        0x031E
#define WM_DWMCOLORIZATIONCOLORCHANGED  0x0320

#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include <shlwapi.h>
#include <richedit.h>
#include <limits.h>
#include <ctype.h>
#include <string>
#include <assert.h>

#include "resource.h"

#ifdef _MSC_VER
#ifdef WM_THEMECHANGED
#undef WM_THEMECHANGED
#endif
#ifdef CDRF_NOTIFYSUBITEMDRAW
#undef CDRF_NOTIFYSUBITEMDRAW
#endif
#endif

/* State of icon with such flag will not be saved, and you must set it manually */
#define MBF_OWNERSTATE        0x04

#include  <win2k.h>
#include  <newpluginapi.h>
#include  <m_imgsrvc.h>
#include  <m_system.h>
#include  <m_database.h>
#include  <m_langpack.h>
#include  <m_button.h>
#include  <m_clist.h>
#include  <m_options.h>
#include  <m_protosvc.h>
#include  <m_utils.h>
#include  <m_skin.h>
#include  <m_contacts.h>
#include  <m_icolib.h>
#include  <m_clc.h>
#include  <m_clui.h>
#include  <m_userinfo.h>
#include  <m_history.h>
#include  <m_addcontact.h>
#include  <m_file.h>
#include  <m_fontservice.h>
#include  <m_acc.h>
#include  <m_chat.h>
#include  <m_protomod.h>
#include  <m_hotkeys.h>
#include  <m_genmenu.h>

extern struct LIST_INTERFACE li;

#define safe_sizeof(a) (unsigned int)((sizeof((a)) / sizeof((a)[0])))

#include "../API/m_ieview.h"
#include "../API/m_popup.h"
#include "../API/m_metacontacts.h"
#include "../API/m_fingerprint.h"
#include "../API/m_nudge.h"
#include "../API/m_folders.h"
#include "../API/m_msg_buttonsbar.h"
#include "../API/m_cln_skinedit.h"
#include "../API/m_flash.h"
#include "../API/m_spellchecker.h"
#include "../API/m_MathModule.h"
#include "../API/m_historyevents.h"
#include "../API/m_buttonbar.h"
#include "../API/m_toptoolbar.h"
#include "../API/m_updater.h"
#include "../API/m_smileyadd.h"

#include "msgdlgutils.h"

#include "msgs.h"
#ifndef __CPP_LEAN
#include "nen.h"
#include "functions.h"
#include "typingnotify.h"
#include "generic_msghandlers.h"
#include "../chat/chat.h"
extern 	NEN_OPTIONS	nen_options;
#endif

#include "themes.h"
#include "globals.h"
#include "mim.h"
#include "sendqueue.h"
#include "taskbar.h"
#include "controls.h"
#include "infopanel.h"
#include "sidebar.h"

#if !defined(_WIN64) && !defined(_USE_32BIT_TIME_T)
	#define _USE_32BIT_TIME_T
#else
	#undef _USE_32BIT_TIME_T
#endif

#if _MSC_VER >= 1500 || defined(__GNUWIN32__)
	#define wEffects wReserved
#endif

typedef struct __paraformat2
{
	UINT	cbSize;
	DWORD	dwMask;
	WORD	wNumbering;
	WORD	wReserved;
	LONG	dxStartIndent;
	LONG	dxRightIndent;
	LONG	dxOffset;
	WORD	wAlignment;
	SHORT	cTabCount;
	LONG	rgxTabs[MAX_TAB_STOPS];
 	LONG	dySpaceBefore;			// Vertical spacing before para
	LONG	dySpaceAfter;			// Vertical spacing after para
	LONG	dyLineSpacing;			// Line spacing depending on Rule
	SHORT	sStyle;					// Style handle
	BYTE	bLineSpacingRule;		// Rule for line spacing (see tom.doc)
	BYTE	bOutlineLevel;			// Outline Level
	WORD	wShadingWeight;			// Shading in hundredths of a per cent
	WORD	wShadingStyle;			// Byte 0: style, nib 2: cfpat, 3: cbpat
	WORD	wNumberingStart;		// Starting value for numbering
	WORD	wNumberingStyle;		// Alignment, Roman/Arabic, (), ), ., etc.
	WORD	wNumberingTab;			// Space bet 1st indent and 1st-line text
	WORD	wBorderSpace;			// Border-text spaces (nbl/bdr in pts)
	WORD	wBorderWidth;			// Pen widths (nbl/bdr in half twips)
	WORD	wBorders;				// Border styles (nibble/border)
} _PARAFORMAT2;

/*
 * tchar-like std::string
 */

typedef std::basic_string<TCHAR> tstring;

extern	HINSTANCE g_hInst;
extern CSkinItem SkinItems[];
extern ContainerWindowData *pFirstContainer, *pLastActiveContainer;

#endif /* __COMMONHEADERS_H */
