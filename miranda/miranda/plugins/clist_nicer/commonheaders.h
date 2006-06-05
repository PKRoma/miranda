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

#if defined( UNICODE ) && !defined( _UNICODE )
#define _UNICODE 
#endif

#define _USE_32BIT_TIME_T

#include <tchar.h>

#ifdef _DEBUG
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#else
#	include <malloc.h>
#endif

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
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
#include <m_database.h>
#include <m_langpack.h>
#include <m_button.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <m_utils.h>
#include <m_skin.h>
#include <m_contacts.h>
#include <m_file.h>
#include <m_addcontact.h>
#include <m_message.h>
#include "m_avatars.h"
#include "extbackg.h"
#include "clc.h"
#include "clist.h"
#include "alphablend.h"
#include "rowheight_funcs.h"
#include "m_genmenu.h"
#include "CLUIFrames/genmenu.h"
#include "m_genmenu.h"
#include "m_cluiframes.h"
#include "m_clui.h"
#include "icolib.h"
#include "m_popup.h"
#include "m_metacontacts.h"
#include "m_fontservice.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// NEEDS TO GO AWAY when we get REAL unicode
// 

#if !defined(_UNICODE)

#define ptszVal pszVal

#define DBWriteContactSettingTString(a, b, c, d) DBWriteContactSettingString((a), (b), (c), (d))
#define DBWriteContactSettingWString(a, b, c, d) DBWriteContactSettingString((a), (b), (c), (d))

#undef DBGetContactSettingTString
#undef DBGetContactSettingWString

#define DBGetContactSettingTString(a,b,c,d) DBGetContactSetting(a,b,c,d)
#define DBGetContactSettingWString(a,b,c,d) DBGetContactSetting(a,b,c,d)

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
extern struct MM_INTERFACE memoryManagerInterface;

#define mir_alloc(n) memoryManagerInterface.mmi_malloc(n)
#define mir_free(ptr) memoryManagerInterface.mmi_free(ptr)
#define mir_realloc(ptr,size) memoryManagerInterface.mmi_realloc(ptr,size)

__forceinline char * mir_strdup(const char *src)
{
	return (src == NULL) ? NULL : strcpy(( char* )mir_alloc( strlen(src)+1 ), src );
}

__forceinline WCHAR* mir_wstrdup(const WCHAR *src)
{
	return (src == NULL) ? NULL : wcscpy(( WCHAR* )mir_alloc(( wcslen(src)+1 )*sizeof( WCHAR )), src );
}

#if defined( _UNICODE )
	#define mir_tstrdup mir_wstrdup
#else
	#define mir_tstrdup mir_strdup
#endif

typedef  int  (__cdecl *pfnDrawAvatar)(HDC hdcOrig, HDC hdcMem, RECT *rc, struct ClcContact *contact, int y, struct ClcData *dat, int selected, WORD cstatus, int rowHeight);
typedef  void (__cdecl *pfnDrawAlpha)(HDC hdcwnd, PRECT rc, DWORD basecolor, BYTE alpha, DWORD basecolor2, BOOL transparent, DWORD FLG_GRADIENT, DWORD FLG_CORNER, DWORD BORDERSTYLE, ImageItem *item);

static char *DBGetString(HANDLE hContact,const char *szModule,const char *szSetting)
{
	char *str=NULL;
	DBVARIANT dbv;
	DBGetContactSetting(hContact,szModule,szSetting,&dbv);
	if(dbv.type==DBVT_ASCIIZ)
		str=mir_strdup(dbv.pszVal);
	DBFreeVariant(&dbv);
	return str;
}

#define safe_sizeof(a) (sizeof((a)) / sizeof((a)[0]))

extern BOOL __forceinline GetItemByStatus(int status, StatusItems_t *retitem);

void GDIp_DrawAlpha(HDC hdcwnd, PRECT rc, DWORD basecolor, BYTE alpha, DWORD basecolor2, BOOL transparent, DWORD FLG_GRADIENT, DWORD FLG_CORNER, DWORD BORDERSTYLE);
void DrawAlpha(HDC hdcwnd, PRECT rc, DWORD basecolor, BYTE alpha, DWORD basecolor2, BOOL transparent, DWORD FLG_GRADIENT, DWORD FLG_CORNER, DWORD BORDERSTYLE, ImageItem *item);
