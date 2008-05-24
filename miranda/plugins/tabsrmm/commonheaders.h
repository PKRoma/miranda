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

#define _USE_32BIT_TIME_T
#define WINVER 0x0501

#define MIRANDA_VER 0x0700

#ifdef __GNUWIN32__

#define OLERENDER_FORMAT 2
#define CFM_BACKCOLOR 0x04000000
#ifndef __TSR_CXX
typedef unsigned short wchar_t;
#endif
#endif

//mad: definition for vc6
#ifndef UINT_MAX
#define UINT_MAX      0xffffffff    /* maximum unsigned int value */
#else
#error "Can't define UINT_MAX"
#endif
//

#define _WIN32_IE 0x0501

#if defined( UNICODE ) && !defined( _UNICODE )
#define _UNICODE
#endif

#include <tchar.h>


#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include <shlwapi.h>
#include <Richedit.h>
#include "resource.h"


#ifdef WM_THEMECHANGED
#undef WM_THEMECHANGED
#endif

#ifdef CDRF_NOTIFYSUBITEMDRAW
#undef CDRF_NOTIFYSUBITEMDRAW
#endif

//MAD
#define MBF_OWNERSTATE        0x04
//State of icon with such flag will not be saved, and you must set it manually  
//

#include "../../include/win2k.h"
#include "../../include/newpluginapi.h"
#include "../../include/m_system.h"
#include "../../include/m_database.h"
#include "../../include/m_langpack.h"
#include "../../include/m_button.h"
#include "../../include/m_clist.h"
#include "../../include/m_options.h"
#include "../../include/m_protosvc.h"
#include "../../include/m_utils.h"
#include "../../include/m_skin.h"
#include "../../include/m_contacts.h"
#include "../../include/m_icolib.h"

#include "../../include/m_clc.h"
#include "../../include/m_clui.h"
#include "../../include/m_userinfo.h"
#include "../../include/m_history.h"
#include "../../include/m_addcontact.h"
#include "../../include/m_file.h"
#include "../../include/m_fontservice.h"

//MAD
#include "buttonsbar.h"
//
//Bolshevik: avatar control
#include "m_acc.h"
//Bolshevik_

extern struct LIST_INTERFACE li;

#define safe_sizeof(a) (unsigned int)((sizeof((a)) / sizeof((a)[0])))

#if !defined(_UNICODE)

#define ptszVal pszVal

#undef DBGetContactSettingTString
#undef DBGetContactSettingWString

#define DBGetContactSettingTString(a,b,c,d) DBGetContactSetting(a,b,c,d)
#define DBGetContactSettingWString(a,b,c,d) DBGetContactSetting(a,b,c,d)

#endif

#include "API/m_ieview.h"
#include "API/m_popup.h"
#include "API/m_metacontacts.h"
#include "API/m_fingerprint.h"
#include "API/m_nudge.h"
#include "API/m_folders.h"
#include "API/m_msg_buttonsbar.h"
#include "API/m_cln_skinedit.h"
#include "API/m_flash.h"
#include "API/m_spellchecker.h"
#include "API/m_MathModule.h"
#include "API/m_historyevents.h"

#ifndef __TSR_CXX
#include "msgs.h"
#include "msgdlgutils.h"
#include "nen.h"
#include "functions.h"
#include "typingnotify.h"
#include "generic_msghandlers.h"
#include "chat/chat.h"

#endif
