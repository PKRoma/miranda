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
#include <mbstring.h>

extern HINSTANCE g_hInst;
extern HANDLE hHookWinPopup;

static unsigned hookNum = 0;
static unsigned serviceNum = 0;
static HANDLE* hHooks = NULL;
static HANDLE* hServices = NULL;

HANDLE HookEvent_Ex(const char *name, MIRANDAHOOK hook) {
	hookNum ++;
	hHooks = (HANDLE *) mir_realloc(hHooks, sizeof(HANDLE) * (hookNum));
	hHooks[hookNum - 1] = HookEvent(name, hook);
	return hHooks[hookNum - 1] ;
}

HANDLE CreateServiceFunction_Ex(const char *name, MIRANDASERVICE service) {
	serviceNum++;
	hServices = (HANDLE *) mir_realloc(hServices, sizeof(HANDLE) * (serviceNum));
	hServices[serviceNum - 1] = CreateServiceFunction(name, service);
	return hServices[serviceNum - 1] ;
}

void UnhookEvents_Ex() {
	int i;
	for (i=0; i<hookNum ; ++i) {
		if (hHooks[i] != NULL) {
			UnhookEvent(hHooks[i]);
		}
	}
	mir_free(hHooks);
	hookNum = 0;
	hHooks = NULL;
}

void DestroyServices_Ex() {
	int i;
	for (i=0; i<serviceNum; ++i) {
		if (hServices[i] != NULL) {
			DestroyServiceFunction(hServices[i]);
		}
	}
	mir_free(hServices);
	serviceNum = 0;
	hServices = NULL;
}


int safe_wcslen(wchar_t *msg, int maxLen) {
    int i;
	for (i = 0; i < maxLen; i++) {
		if (msg[i] == (wchar_t)0)
			return i;
	}
	return 0;
}

TCHAR *a2tcp(const char *text, int cp) {
	if ( text != NULL ) {
	#if defined ( _UNICODE )
		int cbLen = MultiByteToWideChar( cp, 0, text, -1, NULL, 0 );
		TCHAR* result = ( TCHAR* )mir_alloc( sizeof(TCHAR)*( cbLen+1 ));
		if ( result == NULL )
			return NULL;
		MultiByteToWideChar(cp, 0, text, -1, result, cbLen);
		return result;
	#else
		return mir_strdup(text);
	#endif
	}
	return NULL;
}

char* u2a( const wchar_t* src, int codepage ) {
	int cbLen = WideCharToMultiByte( codepage, 0, src, -1, NULL, 0, NULL, NULL );
	char* result = ( char* )mir_alloc( cbLen+1 );
	if ( result == NULL )
		return NULL;

	WideCharToMultiByte( codepage, 0, src, -1, result, cbLen, NULL, NULL );
	result[ cbLen ] = 0;
	return result;
}

wchar_t* a2u( const char* src, int codepage ) {
	int cbLen = MultiByteToWideChar( codepage, 0, src, -1, NULL, 0 );
	wchar_t* result = ( wchar_t* )mir_alloc( sizeof(wchar_t)*(cbLen+1));
	if ( result == NULL )
		return NULL;
	MultiByteToWideChar( codepage, 0, src, -1, result, cbLen );
	result[ cbLen ] = 0;
	return result;
}

TCHAR *a2t(const char *text) {
	if ( text == NULL )
		return NULL;

	#if defined ( _UNICODE )
		return a2tcp(text, CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 ));
	#else
		return a2tcp(text, CP_ACP);
	#endif
}

char* t2a( const TCHAR* src ) {
	#if defined( _UNICODE )
		return u2a( src, CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 ) );
	#else
		return mir_strdup( src );
	#endif
}

char* t2acp( const TCHAR* src, int codepage ) {
	#if defined( _UNICODE )
		return u2a( src, codepage );
	#else
		return mir_strdup( src );
	#endif
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

const char *filename = "scriver.log";

void logInfo(const char *fmt, ...) {
	SYSTEMTIME time;
	char *str;
	va_list vararg;
	int strsize;
	FILE *flog=fopen(filename,"at");
	if (flog!=NULL) {
		GetLocalTime(&time);
    	va_start(vararg, fmt);
    	str = (char *) malloc(strsize=2048);
    	while (_vsnprintf(str, strsize, fmt, vararg) == -1)
    		str = (char *) realloc(str, strsize+=2048);
    	va_end(vararg);
    	fprintf(flog,"%04d-%02d-%02d %02d:%02d:%02d,%03d [%s]",time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,time.wMilliseconds, "INFO");
		fprintf(flog,"  %s\n",str);
    	free(str);
		fclose(flog);
	}
}

int GetRichTextLength(HWND hwnd, int codepage, BOOL inBytes) {
	GETTEXTLENGTHEX gtl;
	gtl.codepage = codepage;
	if (inBytes) {
		gtl.flags = GTL_NUMBYTES;
	} else {
		gtl.flags = GTL_NUMCHARS;
	}
	gtl.flags |= GTL_PRECISE | GTL_USECRLF;
	return (int) SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
}


TCHAR *GetRichText(HWND hwnd, int codepage) {
	GETTEXTEX  gt = {0};
	TCHAR *textBuffer = NULL;
	int textBufferSize;
#if defined( _UNICODE )
	codepage = 1200;
#endif
	textBufferSize = GetRichTextLength(hwnd, codepage, TRUE);
	if (textBufferSize > 0) {
		textBufferSize += sizeof(TCHAR);
		textBuffer = (TCHAR *) mir_alloc(textBufferSize);
		gt.cb = textBufferSize;
		gt.flags = GT_USECRLF;
		gt.codepage = codepage;
		SendMessage(hwnd, EM_GETTEXTEX, (WPARAM) &gt, (LPARAM) textBuffer);
	}
	return textBuffer;
}

char *GetRichTextEncoded(HWND hwnd, int codepage) {
#if defined( _UNICODE )
	TCHAR *textBuffer = GetRichText(hwnd, codepage);
	char *textUtf = NULL;
	if (textBuffer != NULL) {
		textUtf = mir_utf8encodeW(textBuffer);
		mir_free(textBuffer);
	}
	return textUtf;
#else
	return GetRichText(hwnd, codepage);
#endif
}

int SetRichTextEncoded(HWND hwnd, const char *text, int codepage) {
	TCHAR *textToSet;
	SETTEXTEX  st;
	st.flags = ST_DEFAULT;
	st.codepage = codepage;
	#ifdef _UNICODE
		st.codepage = 1200;
		textToSet = mir_utf8decodeW(text);
	#else
		textToSet = text;
	#endif
	SendMessage(hwnd, EM_SETTEXTEX, (WPARAM) &st, (LPARAM)textToSet);
	#ifdef _UNICODE
		mir_free(textToSet);
	#endif
	return GetRichTextLength(hwnd, st.codepage, FALSE);
}

int SetRichTextRTF(HWND hwnd, const char *text) {
	TCHAR *textToSet;
	SETTEXTEX  st;
	st.flags = ST_DEFAULT;
	st.codepage = CP_ACP;
	SendMessage(hwnd, EM_SETTEXTEX, (WPARAM) &st, (LPARAM)text);
	return GetRichTextLength(hwnd, st.codepage, FALSE);
}

static DWORD CALLBACK RichTextStreamCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
	static DWORD dwRead;
	char ** ppText = (char **) dwCookie;

	if (*ppText == NULL) {
		*ppText = mir_alloc(cb + 1);
		memcpy(*ppText, pbBuff, cb);
		(*ppText)[cb] = 0;
		*pcb = cb;
		dwRead = cb;
	}
	else {
		char  *p = mir_alloc(dwRead + cb + 1);
		memcpy(p, *ppText, dwRead);
		memcpy(p+dwRead, pbBuff, cb);
		p[dwRead + cb] = 0;
		mir_free(*ppText);
		*ppText = p;
		*pcb = cb;
		dwRead += cb;
	}

	return 0;
}

char* GetRichTextRTF(HWND hwndDlg)
{
	EDITSTREAM stream;
	char* pszText = NULL;
	DWORD dwFlags;

	if (hwndDlg == 0)
		return NULL;

	ZeroMemory(&stream, sizeof(stream));
	stream.pfnCallback = RichTextStreamCallback;
	stream.dwCookie = (DWORD) &pszText; // pass pointer to pointer

	#if defined( _UNICODE )
		dwFlags = SF_RTFNOOBJS | SFF_PLAINRTF | SF_USECODEPAGE | (CP_UTF8 << 16);
	#else
		dwFlags = SF_RTFNOOBJS | SFF_PLAINRTF;
	#endif
	SendMessage(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE), EM_STREAMOUT, dwFlags, (LPARAM) & stream);
	return pszText; // pszText contains the text
}


void InputAreaContextMenu(HWND hwnd, WPARAM wParam, LPARAM lParam, HANDLE hContact) {

	HMENU hMenu, hSubMenu;
	POINT pt;
	CHARRANGE sel, all = { 0, -1 };
	MessageWindowPopupData mwpd;
	int selection;

	hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT));
	hSubMenu = GetSubMenu(hMenu, 2);
	CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
	SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) & sel);
	if (sel.cpMin == sel.cpMax) {
		EnableMenuItem(hSubMenu, IDM_CUT, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hSubMenu, IDM_COPY, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hSubMenu, IDM_DELETE, MF_BYCOMMAND | MF_GRAYED);
	}
	if (!SendMessage(hwnd, EM_CANUNDO, 0, 0)) {
		EnableMenuItem(hSubMenu, IDM_UNDO, MF_BYCOMMAND | MF_GRAYED);
	}
	if (!SendMessage(hwnd, EM_CANREDO, 0, 0)) {
		EnableMenuItem(hSubMenu, IDM_REDO, MF_BYCOMMAND | MF_GRAYED);
	}
	if (!SendMessage(hwnd, EM_CANPASTE, 0, 0)) {
		EnableMenuItem(hSubMenu, IDM_PASTE, MF_BYCOMMAND | MF_GRAYED);
	}
	if (lParam == 0xFFFFFFFF) {
		SendMessage(hwnd, EM_POSFROMCHAR, (WPARAM) & pt, (LPARAM) sel.cpMax);
		ClientToScreen(hwnd, &pt);
	}
	else {
		pt.x = (short) LOWORD(lParam);
		pt.y = (short) HIWORD(lParam);
	}

	// First notification
	mwpd.cbSize = sizeof(mwpd);
	mwpd.uType = MSG_WINDOWPOPUP_SHOWING;
	mwpd.uFlags = MSG_WINDOWPOPUP_INPUT;
	mwpd.hContact = hContact;
	mwpd.hwnd = hwnd;
	mwpd.hMenu = hSubMenu;
	mwpd.selection = 0;
	mwpd.pt = pt;
	NotifyEventHooks(hHookWinPopup, 0, (LPARAM)&mwpd);

	selection = TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, GetParent(hwnd), NULL);

	// Second notification
	mwpd.selection = selection;
	mwpd.uType = MSG_WINDOWPOPUP_SELECTED;
	NotifyEventHooks(hHookWinPopup, 0, (LPARAM)&mwpd);

	switch (selection) {
	case IDM_UNDO:
		SendMessage(hwnd, WM_UNDO, 0, 0);
		break;
	case IDM_REDO:
		SendMessage(hwnd, EM_REDO, 0, 0);
		break;
	case IDM_CUT:
		SendMessage(hwnd, WM_CUT, 0, 0);
		break;
	case IDM_COPY:
		SendMessage(hwnd, WM_COPY, 0, 0);
		break;
	case IDM_PASTE:
		SendMessage(hwnd, EM_PASTESPECIAL, CF_TEXT, 0);
		break;
	case IDM_DELETE:
		SendMessage(hwnd, EM_REPLACESEL, TRUE, 0);
		break;
	case IDM_SELECTALL:
		SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) & all);
		break;
	case IDM_CLEAR:
		SetWindowText(hwnd, _T( "" ));
		break;
	}
	DestroyMenu(hMenu);
	//PostMessage(hwnd, WM_KEYUP, 0, 0 );
}

int InputAreaShortcuts(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	BOOL isShift = GetKeyState(VK_SHIFT) & 0x8000;
	BOOL isAlt = GetKeyState(VK_MENU) & 0x8000;
	BOOL isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) && !isAlt;
	switch (msg) {
		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE && isShift) { // shift+ESC
				ShowWindow(GetParent(GetParent(hwnd)), SW_MINIMIZE);
				return FALSE;
			}
			if (wParam == VK_TAB && isCtrl && isShift) { // ctrl-shift tab
				SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATEPREV, 0, (LPARAM)GetParent(hwnd));
				return FALSE;
			}
			if (wParam == VK_TAB && isCtrl) { // ctrl tab
				SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATENEXT, 0, (LPARAM)GetParent(hwnd));
				return FALSE;
			}
			if (wParam == VK_PRIOR && isCtrl) { // page up
				SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATEPREV, 0, (LPARAM)GetParent(hwnd));
				return FALSE;
			}
			if (wParam == VK_NEXT && isCtrl) { // page down
				SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATENEXT, 0, (LPARAM)GetParent(hwnd));
				return FALSE;
			}
			if (wParam == VK_F4 && isCtrl && !isShift) { // ctrl + F4
				SendMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
				return FALSE;
			}
			if (wParam == 'A' && isCtrl) { //ctrl-a; select all
				SendMessage(hwnd, EM_SETSEL, 0, -1);
				return FALSE;
			}
			if (wParam == 'I' && isCtrl) { // ctrl-i (italics)
				return FALSE;
			}
			if (wParam == 'L' && isCtrl) { // ctrl-l clear log
				SendMessage(GetParent(hwnd), DM_CLEARLOG, 0, 0);
				return FALSE;
			}
			if (wParam == VK_SPACE && isCtrl) // ctrl-space (paste clean text)
				return FALSE;

			if (wParam == 'T' && isCtrl && isShift) {     // ctrl-shift-t
				SendMessage(GetParent(GetParent(hwnd)), DM_SWITCHTOOLBAR, 0, 0);
				return FALSE;
			}
			if (wParam == 'S' && isCtrl && isShift) {     // ctrl-shift-s
				SendMessage(GetParent(GetParent(hwnd)), DM_SWITCHSTATUSBAR, 0, 0);
				return FALSE;
			}
			if (wParam == 'M' && isCtrl && isShift) {     // ctrl-shift-m
				SendMessage(GetParent(GetParent(hwnd)), DM_SWITCHTITLEBAR, 0, 0);
				return FALSE;
			}
			if (wParam == 'W' && isCtrl && !isAlt) {     // ctrl-w; close
				SendMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
				return FALSE;
			}
			if (wParam == 'R' && isCtrl && isShift) {     // ctrl-shift-r
				SendMessage(GetParent(hwnd), DM_SWITCHRTL, 0, 0);
				return FALSE;
			}
		}
		break;

	}
	return -1;

}
