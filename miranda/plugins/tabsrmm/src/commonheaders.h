/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project,
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

#if defined( UNICODE ) && !defined( _UNICODE )
	#define _UNICODE
#endif

#define WINVER 0x0600
#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0501

#define MIRANDA_VER 0x0700

#include <m_stdhdr.h>

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include <shlwapi.h>
#include <richedit.h>
#include <limits.h>
#include <ctype.h>

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

extern struct LIST_INTERFACE li;

#define safe_sizeof(a) (unsigned int)((sizeof((a)) / sizeof((a)[0])))

#if !defined(_UNICODE)

#define ptszVal pszVal

#undef DBGetContactSettingTString
#undef DBGetContactSettingWString

#define DBGetContactSettingTString(a,b,c,d) DBGetContactSetting(a,b,c,d)
#define DBGetContactSettingWString(a,b,c,d) DBGetContactSetting(a,b,c,d)

#endif

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

#ifndef __CPP_LEAN
#include "msgs.h"
#include "nen.h"
#include "functions.h"
#include "typingnotify.h"
#include "generic_msghandlers.h"
#include "../chat/chat.h"
#endif

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

