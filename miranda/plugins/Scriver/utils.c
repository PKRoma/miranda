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
#include <winsock.h>
#include <m_netlib.h>


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
		textToSet = (char *)text;
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

TCHAR *GetRichTextWord(HWND hwnd, POINTL *ptl)
{
	TCHAR* pszWord = NULL;
	long iCharIndex, start, end, iRes;
	iCharIndex = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM)ptl);
	if (iCharIndex >= 0) {
		start = SendMessage(hwnd, EM_FINDWORDBREAK, WB_LEFT, iCharIndex);//-iChars;
		end = SendMessage(hwnd, EM_FINDWORDBREAK, WB_RIGHT, iCharIndex);//-iChars;
		if (end - start > 0) {
			TEXTRANGE tr;
			CHARRANGE cr;
			static TCHAR szTrimString[] = _T(":;,.!?\'\"><()[]- \r\n");
			ZeroMemory(&tr, sizeof(TEXTRANGE));
			pszWord = mir_alloc(sizeof(TCHAR) * (end - start + 1));
			cr.cpMin = start;
			cr.cpMax = end;
			tr.chrg = cr;
			tr.lpstrText = pszWord;
			iRes = SendMessage(hwnd, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
			if (iRes > 0) {
				int iLen = lstrlen(pszWord)-1;
				while(iLen >= 0 && _tcschr(szTrimString, pszWord[iLen])) {
					pszWord[iLen] = _T('\0');
					iLen--;
				}
			}
		}
	}
	return pszWord;
}

void SearchWord(TCHAR * word, int engine)
{
	char szURL[4096];
	if (word && word[0]) {
		char *wordUTF = mir_utf8encodeT(word);
		char *wordURL = (char *)CallService(MS_NETLIB_URLENCODE, 0, (LPARAM)wordUTF);
		mir_free(wordUTF);
		switch (engine) {
			case SEARCHENGINE_WIKIPEDIA:
				mir_snprintf( szURL, sizeof( szURL ), "http://en.wikipedia.org/wiki/%s", wordURL );
				break;
			case SEARCHENGINE_YAHOO:
				mir_snprintf( szURL, sizeof( szURL ), "http://search.yahoo.com/search?p=%s&ei=UTF-8", wordURL );
				break;
			case SEARCHENGINE_GOOGLE:
			default:
				mir_snprintf( szURL, sizeof( szURL ), "http://www.google.com/search?q=%s&ie=utf-8&oe=utf-8", wordURL );
				break;
		}
		HeapFree(GetProcessHeap(), 0, wordURL);
		CallService(MS_UTILS_OPENURL, 1, (LPARAM) szURL);
	}
}

HDWP ResizeToolbar(HWND hwnd, HDWP hdwp, int width, int vPos, int height, int cControls, const UINT * controls, UINT *controlWidth, UINT *controlSpacing, char *controlAlignment, int controlVisibility)
{
	int i;
	int lPos = 0;
	int rPos = width;
	for (i = 0; i < cControls ; i++) {
		if (!controlAlignment[i] && (controlVisibility & (1 << i))) {
			lPos += controlSpacing[i];
			hdwp = DeferWindowPos(hdwp, GetDlgItem(hwnd, controls[i]), 0, lPos, vPos, controlWidth[i], height, SWP_NOZORDER);
			lPos += controlWidth[i];
		}
	}
	for (i = cControls - 1; i >=0; i--) {
		if (controlAlignment[i] && (controlVisibility & (1 << i))) {
			rPos -= controlSpacing[i] + controlWidth[i];
			hdwp = DeferWindowPos(hdwp, GetDlgItem(hwnd, controls[i]), 0, rPos, vPos, controlWidth[i], height, SWP_NOZORDER);
		}
	}
	return hdwp;
}


void AppendToBuffer(char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
{
	va_list va;
	int charsDone;

	va_start(va, fmt);
	for (;;) {
		charsDone = _vsnprintf(*buffer + *cbBufferEnd, *cbBufferAlloced - *cbBufferEnd, fmt, va);
		if (charsDone >= 0)
			break;
		*cbBufferAlloced += 1024;
		*buffer = (char *) mir_realloc(*buffer, *cbBufferAlloced);
	}
	va_end(va);
	*cbBufferEnd += charsDone;
}
