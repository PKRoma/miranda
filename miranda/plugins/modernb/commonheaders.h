#ifndef commonheaders_h__
#define commonheaders_h__

/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project,
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

#include "m_stdhdr.h"

#if defined (_DEBUG)
#define TRACE(str)  { log0(str); }
#else
  #define TRACE(str)
#endif

#if defined (_DEBUG)
  #define TRACEVAR(str,n) { log1(str,n); }
#else
  #define TRACEVAR(str,n)
#endif

#if defined (_DEBUG)
#define TRACET(str) OutputDebugString(str)
#else
#define TRACET(str)
#endif

//   OutputDebugString(str)

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
#include <win2k.h>


#include "hdr/modern_global_structure.h"

#include <newpluginapi.h>
#include <m_system.h>
#include <m_utils.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_button.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <m_clist.h>
#include <m_clistint.h>
#include <m_skin.h>
#include <m_contacts.h>
#include <m_plugins.h>
#include "m_genmenu.h"
#include "m_clui.h"
#include "m_clc.h"
#include "clc.h"
#include "clist.h"
#include "m_icolib.h"
#include <m_userinfo.h>
#include "./hdr/cluiframes.h"
#include "./m_api/m_cluiframes.h"
#include "./m_api/m_metacontacts.h"
#include "./m_api/m_skin_eng.h"
#include <m_file.h>
#include <m_addcontact.h>

#include "rowheight_funcs.h"
#include "cache_funcs.h"

#include "log.h"

#include "richedit.h"
#include "./m_api/m_variables.h"
#include "m_avatars.h"
#include "./m_api/m_smileyadd.h"

#include "./m_api/m_xpTheme.h"

// module name of MetaContacts plugin
extern char *meta_module;

//macros to free data and set it pointer to NULL
#define mir_free_and_nill(x) {mir_free(x); x=NULL;}
// shared vars

#define CLUI_FRAME_AUTOHIDENOTIFY  512
#define CLUI_FRAME_SHOWALWAYS      1024

extern struct LIST_INTERFACE li;
extern struct MM_INTERFACE memoryManagerInterface;

//#define alloc(n) mir_alloc(n)

#define MAX_REGS(_A_) (sizeof(_A_)/sizeof(_A_[0]))

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW 0x00020000
#endif

extern BOOL __cdecl strstri(const char *a, const char *b);
extern BOOL __cdecl mir_bool_strcmpi(const char *a, const char *b);
extern int __cdecl mir_strcmp (const char *a, const char *b);
extern int __cdecl mir_strlen (const char *a);
extern int __cdecl mir_strcmpi(const char *a, const char *b);
extern int __cdecl mir_tstrcmpi(const TCHAR *a, const TCHAR *b);
//extern __inline void *mir_calloc( size_t num, size_t size );

extern DWORD exceptFunction(LPEXCEPTION_POINTERS EP);

#undef HookEvent
#undef UnhookEvent

#ifdef _DEBUG
#define HookEvent(a,b)  mod_HookEvent(a,b,__FILE__,__LINE__)
#else /* _DEBUG */
#define HookEvent(a,b)  mod_HookEvent(a,b)
#endif /* _DEBUG */

#define UnhookEvent(a)  mod_UnhookEvent(a)

extern HANDLE mod_HookEvent(char *EventID, MIRANDAHOOK HookProc
               #ifdef _DEBUG
                            , char * file, int line);
                #else
                            );
                #endif

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
#define MS_BACKGROUNDCONFIG_REGISTER "ModernBkgrCfg/Register"

//
//  Notification about changed background
//  wParam = ModuleName
//  lParam = 0
#define ME_BACKGROUNDCONFIG_CHANGED "ModernBkgrCfg/Changed"



HBITMAP ske_CreateDIB32(int cx, int cy);
BOOL ske_SetRectOpaqueOpt(HDC memdc,RECT *fr, BOOL force);
BOOL ske_SetRgnOpaqueOpt(HDC memdc,HRGN hrgn, BOOL force);

extern void InitDisplayNameCache(void);
extern void FreeDisplayNameCache();
extern int CLUI_ShowWindowMod(HWND hwnd, int cmd);

#ifdef UNICODE
	#define GCMDF_TCHAR_MY GCMDF_TCHAR|CNF_UNICODE
#else
	#define GCMDF_TCHAR_MY 0
#endif

#ifndef LWA_COLORKEY
#define LWA_COLORKEY            0x00000001
#endif

#ifndef AC_SRC_ALPHA
#define AC_SRC_ALPHA            0x01
#endif

//#ifdef _DEBUG
//#define DeleteObject(a) DebugDeleteObject(a)
//#endif

#define strsetA(a,b) {if (a) mir_free_and_nill(a); a=mir_strdup(b);}
#define strsetT(a,b) {if (a) mir_free_and_nill(a); a=mir_tstrdup(b);}

extern void TRACE_ERROR();
extern BOOL DebugDeleteObject(HGDIOBJ a);
extern BOOL mod_DeleteDC(HDC hdc);
extern BOOL ske_ResetTextEffect(HDC hdc);
extern BOOL ske_SelectTextEffect(HDC hdc, BYTE EffectID, DWORD FirstColor, DWORD SecondColor);
#define GLOBAL_PROTO_NAME "global_connect"
extern void IvalidateDisplayNameCache(DWORD mode);

void FreeAndNil( void **p );

extern SortedList *clistCache;

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


#define STATE_NORMAL 0
#define STATE_PREPEARETOEXIT 1
#define STATE_EXITING 2
#define MirandaExiting() ((g_CluiData.bSTATE>STATE_NORMAL))



extern __inline char * strdupn(const char * src, int len);

#define SKINBUTTONCLASS _T("MirandaSkinButtonClass")

#define SORTBY_NAME	   0
#define SORTBY_STATUS  1
#define SORTBY_LASTMSG 2
#define SORTBY_PROTO   3
#define SORTBY_RATE    4
#define SORTBY_NAME_LOCALE 5
#define SORTBY_NOTHING	10

#define DT_FORCENATIVERENDER   0x10000000

#define _BOOL(a) (a != 0)

/* modern_animated_avatars.c */
int AniAva_InitModule();								   // HAVE TO BE AFTER GDI+ INITIALIZED
int AniAva_UnloadModule();
int AniAva_UpdateOptions();								   //reload options, //hot enable/disable engine

int AniAva_AddAvatar(HANDLE hContact, TCHAR * szFilename, int width, int heigth);  // adds avatars to be displayed
int AniAva_SetAvatarPos(HANDLE hContact, RECT * rc, int overlayIdx, BYTE bAlpha);	   // update avatars pos
int AniAva_InvalidateAvatarPositions(HANDLE hContact);	   // reset positions of avatars to be drawn (still be painted at same place)
int AniAva_RemoveInvalidatedAvatars();					   // all avatars without validated position will be stop painted and probably removed
int AniAva_RemoveAvatar(HANDLE hContact);				   // remove avatar
int AniAva_RedrawAllAvatars(BOOL updateZOrder);			   // request to repaint all
void AniAva_UpdateParent();


#define CCI_NAME			1
#define CCI_GROUP			(1<<1)
#define CCI_PROTO			(1<<2)
#define CCI_STATUS			(1<<3)
#define CCI_LINES			(1<<4)
#define CCI_HIDDEN			(1<<4)
#define CCI_NOHIDEOFFLINE	(1<<5)
#define CCI_NOPROTO			(1<<6)
#define CCI_HIDESUBCONTACT	(1<<7)
#define CCI_I				(1<<8)
#define CCI_APPARENT		(1<<9)
#define CCI_NOTONLIST		(1<<10)
#define CCI_IDLETS			(1<<11)
#define CCI_CCONTACT		(1<<12)
#define CCI_EXPAND			(1<<13)
#define CCI_UNKNOWN			(1<<14)
#define CCI_TIME			(1<<15)
#define CCI_OTHER			~( CCI_NAME|CCI_GROUP|CCI_PROTO|CCI_STATUS|CCI_LINES|CCI_TIME )
#define CCI_ALL				(0xFFFFFFFF)

void CListSettings_FreeCacheItemData(pdisplayNameCacheEntry pDst);
int CLUI_SyncGetPDNCE(WPARAM wParam, LPARAM lParam);
WORD pdnce___GetStatus(pdisplayNameCacheEntry pdnce);
void pdnce___SetStatus( pdisplayNameCacheEntry pdnce, WORD wStatus );

/* move to list module */
typedef void (*ItemDestuctor)(void*);


void li_ListDestruct(SortedList *pList, ItemDestuctor pItemDestructor);
void li_RemoveDestruct(SortedList *pList, int index, ItemDestuctor pItemDestructor);
void li_RemovePtrDestruct(SortedList *pList, void * ptr, ItemDestuctor pItemDestructor);
void li_SortList(SortedList *pList, FSortFunc pSortFunct);

#define mir_safe_free(a) if(a) mir_free(a)

#ifdef _UNICODE
#define mir_t2a(s) mir_u2a(s)
#define mir_a2t(s) mir_a2u(s)
#define mir_t2u(s) mir_wstrdup(s)
#define mir_u2t(s) mir_wstrdup(s)
#else
#define mir_t2a(s) mir_strdup(s)
#define mir_a2t(s) mir_strdup(s)
#define mir_t2u(s) mir_a2u(s)
#define mir_u2t(s) mir_u2a(s)
#endif

#endif // commonheaders_h__
