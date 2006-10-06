/*
Scriver

Copyright 2000-2003 Miranda ICQ/IM project,
Copyright 2005 Piotr Piastucki

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
#ifndef UTILS_H
#define UTILS_H

extern struct MM_INTERFACE memoryManagerInterface;
#define mir_alloc(n) memoryManagerInterface.mmi_malloc(n)
#define mir_free(ptr) memoryManagerInterface.mmi_free(ptr)
#define mir_realloc(ptr,size) memoryManagerInterface.mmi_realloc(ptr,size)

static __inline char * mir_strdup(const char *src)
{
	return (src == NULL) ? NULL : strcpy(( char* )mir_alloc( strlen(src)+1 ), src );
}

static __inline WCHAR* mir_wstrdup(const WCHAR *src)
{
	return (src == NULL) ? NULL : wcscpy(( WCHAR* )mir_alloc(( wcslen(src)+1 )*sizeof( WCHAR )), src );
}
#if defined( _UNICODE )
	#define mir_tstrdup mir_wstrdup
#else
	#define mir_tstrdup mir_strdup
#endif
extern int IsUnicodeMIM();
extern int safe_wcslen(wchar_t *msg, int maxLen) ;
extern TCHAR *a2t(const char *text);
extern TCHAR *a2tl(const char *text, int textlen);
extern TCHAR *a2tlcp(const char *text, int textlen, int cp);

#endif
