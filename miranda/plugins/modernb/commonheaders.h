/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project,
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
#ifndef _COMMON_HEADERS_H_
#define _COMMON_HEADERS_H_ 1



#if defined(UNICODE)
#define _UNICODE 1
#define UNICODE_AWARE 1
#else
	#define UNICODE_AWARE 0
#endif

//#include "AggressiveOptimize.h"

#include <malloc.h>

#ifdef _DEBUG
#	define _CRTDBG_MAP_ALLOC
#	include <stdlib.h>
//#	include <crtdbg.h>
#endif

#if defined (_DEBUG)
  #define TRACE(str)  OutputDebugStringA(str)
#else
  #define TRACE(str)
#endif

#if defined (_DEBUG)
  #define TRACEVAR(str,n) {char buf[255]; _snprintf(buf,sizeof(buf),str,n); OutputDebugStringA(buf);}
#else
  #define TRACEVAR(str,n)
#endif

#if defined (_DEBUG)
#define TRACET(str) OutputDebugString(str)
#else
#define TRACET(str)
#endif

//   OutputDebugString(str)

#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0501
#include <windows.h>

#include <commctrl.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include <io.h>
#include <string.h>
#include <direct.h>
#include "resource.h"
#include "forkthread.h"
#include <win2k.h>

#include <newpluginapi.h>
#include <m_system.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_button.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <m_clist.h>
#include <m_clistint.h>
#include <m_utils.h>
#include <m_skin.h>
#include <m_contacts.h>
#include <m_plugins.h>
#include "m_genmenu.h"
#include "genmenu.h"
#include "m_clui.h"
#include "m_clc.h"
#include "clc.h"
#include "clist.h"
#include "m_icolib.h"
#include <m_userinfo.h>
#include ".\CLUIFrames\cluiframes.h"
#include ".\CLUIFrames\m_cluiframes.h"
#include  "m_metacontacts.h"
#include "m_skin_eng.h"
//#include "BkgrCfg.h"
#include <m_file.h>
#include <m_addcontact.h>

#include "rowheight_funcs.h"
#include "cache_funcs.h"

#include "log.h"

#include "richedit.h"
#include "m_variables.h"
#include "m_avatars.h"
#include "m_smileyadd.h"

// shared vars
extern HINSTANCE g_hInst;

/* most free()'s are invalid when the code is executed from a dll, so this changes
 all the bad free()'s to good ones, however it's still incorrect code. The reasons for not
 changing them include:

  * DBFreeVariant has a CallService() lookup
  * free() is executed in some large loops to do with clist creation of group data
  * easy search and replace

*/
typedef  struct _menuProto 
{
  char *szProto;
  HANDLE menuID;
  HANDLE hasAdded;
} MenuProto;

extern struct LIST_INTERFACE li;
extern struct MM_INTERFACE memoryManagerInterface;

#define alloc(n) mir_alloc(n)

#define MAX_REGS(_A_) (sizeof(_A_)/sizeof(_A_[0]))


#define mir_alloc(n) memoryManagerInterface.mmi_malloc(n)
#define mir_free(ptr) { mir_free_proxy(ptr); ptr=NULL; }
#define mir_realloc(ptr,size) memoryManagerInterface.mmi_realloc(ptr,size)

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW 0x00020000
#endif

extern int mir_realloc_proxy(void *ptr,int size);
extern int mir_free_proxy(void *ptr);
extern BOOL __cdecl strstri(const char *a, const char *b);
extern BOOL __cdecl mir_bool_strcmpi(const char *a, const char *b);
extern int __cdecl mir_strcmp (const char *a, const char *b);
extern int __cdecl mir_strlen (const char *a);
extern int __cdecl mir_strcmpi(const char *a, const char *b);
extern int __cdecl mir_tstrcmpi(const TCHAR *a, const TCHAR *b);
extern __inline void *mir_calloc( size_t num, size_t size );
extern __inline char * mir_strdup(const char * src);
extern __inline wchar_t * mir_strdupW(const wchar_t * src);
#ifdef UNICODE
	#define mir_tstrdup(a) mir_strdupW(a)
#else
	#define mir_tstrdup(a) mir_strdup(a)
#endif


char *DBGetStringA(HANDLE hContact,const char *szModule,const char *szSetting);
extern wchar_t *DBGetStringW(HANDLE hContact,const char *szModule,const char *szSetting);
extern TCHAR *DBGetStringT(HANDLE hContact,const char *szModule,const char *szSetting);
extern DWORD exceptFunction(LPEXCEPTION_POINTERS EP);

#undef HookEvent
#undef UnhookEvent

#define HookEvent(a,b)  mod_HookEvent(a,b)
#define UnhookEvent(a)  mod_UnhookEvent(a)

extern HANDLE mod_HookEvent(char *EventID,MIRANDAHOOK HookProc);
extern int mod_UnhookEvent(HANDLE hHook);
extern int UnhookAll();

#ifndef MYCMP
#define MYCMP 1
#define strcmp(a,b) mir_strcmp(a,b)
#define strlen(a) mir_strlen(a)
#endif

//  Register of plugin's user
//
//  wParam = (WPARAM)szSetting - string that describes a user
//           format: Category/ModuleName,
//           eg: "Contact list background/CLUI",
//               "Status bar background/StatusBar"
//  lParam = (LPARAM)dwFlags
//
//#define MS_BACKGROUNDCONFIG_REGISTER "BkgrCfg/Register"

//
//  Notification about changed background
//  wParam = ModuleName
//  lParam = 0
#define ME_BACKGROUNDCONFIG_CHANGED "BkgrCfg/Changed"



HBITMAP CreateBitmap32(int cx, int cy);
extern void InitDisplayNameCache(void);
extern void FreeDisplayNameCache();
extern int ShowWindowNew(HWND hwnd, int cmd);

#ifdef UNICODE
	#define GCMDF_TCHAR_MY GCMDF_TCHAR|CNF_UNICODE
#else
	#define GCMDF_TCHAR_MY 0
#endif

extern char* Utf8EncodeUcs2( const wchar_t* src );
extern void Utf8Decode( char* str, wchar_t** ucs2 );
#endif

#ifndef LWA_COLORKEY
#define LWA_COLORKEY            0x00000001
#endif

#ifndef AC_SRC_ALPHA
#define AC_SRC_ALPHA            0x01
#endif

#ifdef _DEBUG
#define DeleteObject(a) DebugDeleteObject(a)
#endif 

#define lockdat 
//EnterCriticalSection(&(dat->lockitemCS))
#define ulockdat 
//LeaveCriticalSection(&(dat->lockitemCS))

#define strsetA(a,b) {if (a) mir_free(a); a=mir_strdup(b);}
#define strsetT(a,b) {if (a) mir_free(a); a=mir_tstrdup(b);}

extern void TRACE_ERROR();
extern BOOL DebugDeleteObject(HGDIOBJ a);
extern BOOL mod_DeleteDC(HDC hdc);
extern BOOL ResetEffect(HDC hdc);
extern BOOL SelectEffect(HDC hdc, BYTE EffectID, DWORD FirstColor, DWORD SecondColor);
#define GLOBAL_PROTO_NAME "global_connect"
extern void IvalidateDisplayNameCache(DWORD mode);
CRITICAL_SECTION cacheSection;
extern SortedList *clistCache;
//#define lockcache {if(clistCache) EnterCriticalSection(&cacheSection); if (cacheSection.RecursionCount>20) DebugBreak();}
//#define ulockcache if(clistCache) LeaveCriticalSection(&cacheSection)
extern void LockCacheItem(HANDLE hContact, char*, int);
extern void UnlockCacheItem(HANDLE hContact);

HICON LoadSmallIconShared(HINSTANCE hInstance, LPCTSTR lpIconName);
HICON LoadSmallIcon(HINSTANCE hInstance, LPCTSTR lpIconName);
BOOL DestroyIcon_protect(HICON icon);
extern BOOL (WINAPI *pfEnableThemeDialogTexture)(HANDLE, DWORD);

#ifndef ETDT_ENABLETAB
#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)
#endif



#define TreeView_InsertItemA(hwnd, lpis) \
	(HTREEITEM)SendMessageA((hwnd), TVM_INSERTITEMA, 0, (LPARAM)(LPTV_INSERTSTRUCTA)(lpis))

#define TreeView_GetItemA(hwnd, pitem) \
	(BOOL)SendMessageA((hwnd), TVM_GETITEMA, 0, (LPARAM)(TV_ITEM *)(pitem))

extern HANDLE hAskStatusMessageThread;
extern HANDLE hGetTextThread;
extern HANDLE hSmoothAnimationThread;
extern HANDLE hFillFontListThread;

#define STATE_NORMAL 0
#define STATE_EXITING 1
extern BYTE g_STATE;
#define MirandaExiting() ((g_STATE==STATE_EXITING) || Miranda_Terminated())
extern BYTE gl_TrimText;