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
enum KB_ACTIONS {KB_PREV_TAB = 1, KB_NEXT_TAB, KB_SWITCHTOOLBAR,
				 KB_SWITCHSTATUSBAR, KB_SWITCHTITLEBAR, KB_MINIMIZE, KB_CLOSE, KB_CLEAR_LOG,
				 KB_TAB1, KB_TAB2, KB_TAB3, KB_TAB4, KB_TAB5, KB_TAB6, KB_TAB7, KB_TAB8, KB_TAB9};

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
	unsigned i;
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
	unsigned i;
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

char* GetRichTextRTF(HWND hwnd)
{
	EDITSTREAM stream;
	char* pszText = NULL;
	DWORD dwFlags;

	if (hwnd == 0)
		return NULL;

	ZeroMemory(&stream, sizeof(stream));
	stream.pfnCallback = RichTextStreamCallback;
	stream.dwCookie = (DWORD) &pszText; // pass pointer to pointer

	#if defined( _UNICODE )
		dwFlags = SF_RTFNOOBJS | SFF_PLAINRTF | SF_USECODEPAGE | (CP_UTF8 << 16);
	#else
		dwFlags = SF_RTFNOOBJS | SFF_PLAINRTF;
	#endif
	SendMessage(hwnd, EM_STREAMOUT, dwFlags, (LPARAM) & stream);
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

int InputAreaShortcuts(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, CommonWindowData *windowData) {
	
	BOOL isShift = GetKeyState(VK_SHIFT) & 0x8000;
	BOOL isAlt = GetKeyState(VK_MENU) & 0x8000;
	BOOL isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) && !isAlt;

	int action;
	MSG amsg;
	amsg.hwnd = hwnd;
	amsg.message = msg;
	amsg.wParam = wParam;
	amsg.lParam = lParam;
	switch (action = CallService(MS_HOTKEY_CHECK, (WPARAM)&amsg, (LPARAM)"Scriver"))
	{
		case KB_PREV_TAB:
			SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATEPREV, 0, (LPARAM)GetParent(hwnd));
			return FALSE;
		case KB_NEXT_TAB:
			SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATENEXT, 0, (LPARAM)GetParent(hwnd));
			return FALSE;
		case KB_SWITCHSTATUSBAR:
			SendMessage(GetParent(GetParent(hwnd)), DM_SWITCHSTATUSBAR, 0, 0);
			return FALSE;
		case KB_SWITCHTITLEBAR:
			SendMessage(GetParent(GetParent(hwnd)), DM_SWITCHTITLEBAR, 0, 0);
			return FALSE;
		case KB_SWITCHTOOLBAR:
			SendMessage(GetParent(GetParent(hwnd)), DM_SWITCHTOOLBAR, 0, 0);
			return FALSE;
		case KB_MINIMIZE:
			ShowWindow(GetParent(GetParent(hwnd)), SW_MINIMIZE);
			return FALSE;
		case KB_CLOSE:
			SendMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
			return FALSE;
		case KB_CLEAR_LOG:
			SendMessage(GetParent(hwnd), DM_CLEARLOG, 0, 0);
			return FALSE;
		case KB_TAB1:
		case KB_TAB2:
		case KB_TAB3:
		case KB_TAB4:
		case KB_TAB5:
		case KB_TAB6:
		case KB_TAB7:
		case KB_TAB8:
		case KB_TAB9:
			SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATEBYINDEX, 0, action - KB_TAB1);
			return FALSE;
	}

	switch (msg) {
		case WM_KEYDOWN:
		{
			if (wParam == VK_TAB && isCtrl && isShift) { // ctrl-shift tab
				SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATEPREV, 0, (LPARAM)GetParent(hwnd));
				return FALSE;
			}
			if (wParam == VK_PRIOR && isCtrl) { // page up
				SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATEPREV, 0, (LPARAM)GetParent(hwnd));
				return FALSE;
			}
			if (wParam == VK_TAB && isCtrl) { // ctrl tab
				SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATENEXT, 0, (LPARAM)GetParent(hwnd));
				return FALSE;
			}
			if (wParam == VK_NEXT && isCtrl) { // page down
				SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATENEXT, 0, (LPARAM)GetParent(hwnd));
				return FALSE;
			}
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
			if (wParam == VK_ESCAPE && isShift) { // shift+ESC
				ShowWindow(GetParent(GetParent(hwnd)), SW_MINIMIZE);
				return FALSE;
			}
			if (wParam == VK_F4 && isCtrl && !isShift) { // ctrl + F4
				SendMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
				return FALSE;
			}
			if (wParam == 'W' && isCtrl && !isAlt) {     // ctrl-w; close
				SendMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
				return FALSE;
			}
			if (wParam == 'L' && isCtrl) { // ctrl-l clear log
				SendMessage(GetParent(hwnd), DM_CLEARLOG, 0, 0);
				return FALSE;
			}
			if (wParam >= '1' && wParam <='9' && isCtrl) {
				SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATEBYINDEX, 0, wParam - '1');
				return 0;
			}
			/*
			if (wParam == 'A' && isCtrl) { //ctrl-a; select all
				SendMessage(hwnd, EM_SETSEL, 0, -1);
				return FALSE;
			}
			*/
			if (wParam == 'I' && isCtrl) { // ctrl-i (italics)
				return FALSE;
			}
			if (wParam == VK_SPACE && isCtrl) // ctrl-space (paste clean text)
				return FALSE;
			if (wParam == 'R' && isCtrl && isShift) {     // ctrl-shift-r
				SendMessage(GetParent(hwnd), DM_SWITCHRTL, 0, 0);
				return FALSE;
			}
			if ((wParam == VK_UP || wParam == VK_DOWN) && isCtrl && (g_dat->flags & SMF_CTRLSUPPORT) && !DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOCLOSE, SRMSGDEFSET_AUTOCLOSE)) {
				if (windowData->cmdList) {
					TCmdList *cmdListNew = NULL;
					if (wParam == VK_UP) {
						if (windowData->cmdListCurrent == NULL) {
							cmdListNew = tcmdlist_last(windowData->cmdList);
							while (cmdListNew != NULL && cmdListNew->temporary) {
								windowData->cmdList = tcmdlist_remove(windowData->cmdList, cmdListNew);
								cmdListNew = tcmdlist_last(windowData->cmdList);
							}
							if (cmdListNew != NULL) {
								char *textBuffer;
								if (windowData->flags & CWDF_RTF_INPUT) {
									 textBuffer = GetRichTextRTF(hwnd);
								} else {
									 textBuffer = GetRichTextEncoded(hwnd, windowData->codePage);
								}
								if (textBuffer != NULL) {
									windowData->cmdList = tcmdlist_append(windowData->cmdList, textBuffer, 20, TRUE);
									mir_free(textBuffer);
								}
							}
						} else if (windowData->cmdListCurrent->prev != NULL) {
							cmdListNew = windowData->cmdListCurrent->prev;
						}
					} else {
						if (windowData->cmdListCurrent != NULL) {
							if (windowData->cmdListCurrent->next != NULL) {
								cmdListNew = windowData->cmdListCurrent->next;
							} else if (!windowData->cmdListCurrent->temporary) {
								SetWindowText(hwnd, _T(""));
							}
						}
					}
					if (cmdListNew != NULL) {
						int iLen;
						SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
						if (windowData->flags & CWDF_RTF_INPUT) {
							iLen = SetRichTextRTF(hwnd, cmdListNew->szCmd);
						} else {
							iLen = SetRichTextEncoded(hwnd, cmdListNew->szCmd, windowData->codePage);
						}
						SendMessage(hwnd, EM_SCROLLCARET, 0,0);
						SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
						RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
						SendMessage(hwnd, EM_SETSEL, iLen, iLen);
						windowData->cmdListCurrent = cmdListNew;
					}
				}
				return FALSE;
			}
		}
		break;
		case WM_SYSKEYDOWN:
		{
			if ((wParam == VK_LEFT) && isAlt) {
				SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATEPREV, 0, (LPARAM)GetParent(hwnd));
				return 0;
			}
			if ((wParam == VK_RIGHT) && isAlt) {
				SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATENEXT, 0, (LPARAM)GetParent(hwnd));
				return 0;
			}
		}
		break;
		case WM_SYSKEYUP:
		{
			if ((wParam == VK_LEFT) && isAlt) {
				return 0;
			}
			if ((wParam == VK_RIGHT) && isAlt) {
				return 0;
			}
		}
		break;

	}

	return -1;

}

void RegisterKeyBindings() {
	int i;
	char strDesc[64], strName[64];
	HOTKEYDESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.cbSize = sizeof(desc);
	desc.pszSection = "Scriver";
	desc.pszName = "Scriver/Nav/Previous Tab";
	desc.pszDescription = "Navigate: Previous Tab";
	desc.lParam = KB_PREV_TAB;
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, VK_TAB);
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL, VK_PRIOR);
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_ALT, VK_LEFT);
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);

	desc.pszName = "Scriver/Nav/Next Tab";
	desc.pszDescription = "Navigate: Next Tab";
	desc.lParam = KB_NEXT_TAB;
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL, VK_TAB);
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL, VK_NEXT);
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_ALT, VK_RIGHT);
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);
	desc.pszName = strName;
	desc.pszDescription = strDesc;
	for (i = 0; i < 9; i++) {
		mir_snprintf(strName, SIZEOF(strName), "Scriver/Nav/Tab %d", i + 1);
		mir_snprintf(strDesc, SIZEOF(strDesc), "Navigate: Tab %d", i + 1);
		desc.lParam = KB_TAB1 + i;
		desc.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL, '1' + i);
		CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);
	}

	desc.pszName = "Scriver/Wnd/Toggle Statusbar";
	desc.pszDescription = "Window: Toggle Statusbar";
	desc.lParam = KB_SWITCHSTATUSBAR;
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'S');
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);

	desc.pszName = "Scriver/Wnd/Toggle Titlebar";
	desc.pszDescription = "Window: Toggle Titlebar";
	desc.lParam = KB_SWITCHTITLEBAR;
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'M');
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);

	desc.pszName = "Scriver/Wnd/Toggle Toolbar";
	desc.pszDescription = "Window: Toggle Toolbar";
	desc.lParam = KB_SWITCHTOOLBAR;
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'T');
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);

	desc.pszName = "Scriver/Wnd/Clear Log";
	desc.pszDescription = "Window: Clear Log";
	desc.lParam = KB_CLEAR_LOG;
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL, 'L');
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);

	desc.pszName = "Scriver/Wnd/Minimize";
	desc.pszDescription = "Window: Minimize";
	desc.lParam = KB_MINIMIZE;
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_SHIFT, VK_ESCAPE);
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);

	desc.pszName = "Scriver/Wnd/Close Tab";
	desc.pszDescription = "Window: Close Tab";
	desc.lParam = KB_CLOSE;
//	desc.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL, VK_F4);
//	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);
	desc.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL, 'W');
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM) &desc);
}
