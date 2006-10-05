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
#include "commonheaders.h"
#include <ctype.h>
#include <malloc.h>
#include <mbstring.h>

int safe_wcslen(wchar_t *msg, int maxLen) {
    int i;
	for (i = 0; i < maxLen; i++) {
		if (msg[i] == (wchar_t)0)
			return i;
	}
	return 0;
}

TCHAR *a2tlcp(const char *text, int textlen, int cp) {
	wchar_t *wtext;
	if ( text == NULL )
		return NULL;
	#if defined ( _UNICODE )
		if (textlen == -1) {
			textlen = strlen(text) + 1;
		}
		wtext = (wchar_t *) malloc(sizeof(wchar_t) * textlen);
		MultiByteToWideChar(cp, 0, text, -1, wtext, textlen);
		return wtext;
	#else
		return _tcsdup(text);
	#endif
}

TCHAR *a2tl(const char *text, int textlen) {
	if ( text == NULL )
		return NULL;

	#if defined ( _UNICODE )
		return a2tlcp(text, textlen, CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 ));
	#else
		return a2tlcp(text, textlen, CP_ACP);
	#endif
}

TCHAR *a2t(const char *text) {
	return a2tl(text, -1);
}

static int mimFlags = 0;

enum MIMFLAGS {
	MIM_CHECKED = 1,
	MIM_UNICODE = 2
};

int IsUnicodeMIM() {
	if (!(mimFlags & MIM_CHECKED)) {
		char str[512];
		mimFlags = MIM_CHECKED;
		CallService(MS_SYSTEM_GETVERSIONTEXT, (WPARAM)500, (LPARAM)(char*)str);
		if(strstr(str, "Unicode")) {
			mimFlags |= MIM_UNICODE;
		}
	}
	return (mimFlags & MIM_UNICODE) != 0;
}

