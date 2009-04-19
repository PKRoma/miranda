/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2004 Miranda ICQ/IM project,
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

#define MIRANDA_VER 0x0700

#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0501

#define  EXTRA_ICON_COUNT	11

#include <m_stdhdr.h>

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
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
#include "forkthread.h"
#include <win2k.h>
#include <newpluginapi.h>
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
#include "m_cln_skinedit.h"
#include "m_avatars.h"
#include "extbackg.h"
#include "clc.h"
#include "clist.h"
#include "alphablend.h"
#include "rowheight_funcs.h"
#include "m_genmenu.h"
#include "m_genmenu.h"
#include "m_cluiframes.h"
#include "m_clui.h"
#include "m_icolib.h"
#include "m_popup.h"
#include "m_metacontacts.h"
#include "m_fontservice.h"

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
typedef  void (__cdecl *pfnDrawAlpha)(HDC hdcwnd, PRECT rc, DWORD basecolor, BYTE alpha, DWORD basecolor2, BOOL transparent, DWORD FLG_GRADIENT, DWORD FLG_CORNER, DWORD BORDERSTYLE, ImageItem *item);

#define safe_sizeof(a) (sizeof((a)) / sizeof((a)[0]))

BOOL __forceinline GetItemByStatus(int status, StatusItems_t *retitem);

void DrawAlpha(HDC hdcwnd, PRECT rc, DWORD basecolor, int alpha, DWORD basecolor2, BOOL transparent, BYTE FLG_GRADIENT, BYTE FLG_CORNER, DWORD BORDERSTYLE, ImageItem *item);

void FreeAndNil( void** );

#if _MSC_VER >= 1500
	#define wEffects wReserved
#endif
