/*
Copyright 2000-2010 Miranda IM project, 
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

#define MIRANDA_VER 0x1000

#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0501

#define WIN32_LEAN_AND_MEAN 

#include <m_stdhdr.h>

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <vssym32.h>

#include <time.h>

#include "resource.h"

#include <win2k.h>

#include <newpluginapi.h>
#include <m_system.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_button.h>
#include <m_clist.h>
#include <m_clc.h>
#include <m_clui.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <m_utils.h>
#include <m_skin.h>
#include <m_contacts.h>
#include <m_userinfo.h>
#include <m_history.h>
#include <m_addcontact.h>
#include <m_message.h>
#include <m_file.h>
#include <m_icolib.h>
#include <m_fontservice.h>
#include <m_timezones.h>

#include "cmdlist.h"
#include "msgs.h"
#include "globals.h"
#include "richutil.h"
#include "version.h"

#if _MSC_VER >= 1500
	#define wEffects wReserved
#endif

extern struct LIST_INTERFACE li;
extern HINSTANCE g_hInst;


