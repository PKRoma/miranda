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

#if defined(UNICODE)
   #define _UNICODE
#endif
#ifdef __GNUWIN32__
#define OLERENDER_FORMAT 2
#define CFM_BACKCOLOR 0x04000000
#ifndef __TSR_CXX
typedef unsigned short wchar_t;
#endif
#endif

#include <tchar.h>

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include "resource.h"
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
