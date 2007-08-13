/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
Copyright ( C ) 2007     Maxim Mluhov
Copyright ( C ) 2007     Victor Pavlychko

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $Source: $
Revision       : $Revision: $
Last change on : $Date: $
Last change by : $Author: $

*/

#include "jabber.h"
#include "resource.h"
#include <richedit.h>

#define JCPF_IN			0x01UL
#define JCPF_OUT		0x02UL
#define JCPF_ERROR		0x04UL

#define JCPF_UTF8		0x08UL
#define JCPF_TCHAR		0x00UL

void JabberConsoleProcessXml(XmlNode *node, DWORD flags);
int JabberMenuHandleConsole(WPARAM wParam, LPARAM lParam);
void JabberConsoleInit();
void JabberConsoleUninit();

/* increment buffer with 1K steps */
#define STRINGBUF_INCREMENT		1024

struct StringBuf
{
	char *buf;
	int size;
	int offset;
	int streamOffset;
};

static void sttAppendBufRaw(StringBuf *buf, char *str);
static void sttAppendBufA(StringBuf *buf, char *str);
#ifdef UNICODE
	static void sttAppendBufW(StringBuf *buf, WCHAR *str);
	#define sttAppendBufT(a,b)		(sttAppendBufW((a),(b)))
#else
	#define sttAppendBufT(a,b)		(sttAppendBufA((a),(b)))
#endif
static void sttEmptyBuf(StringBuf *buf);

#define RTF_HEADER	\
			"{\\rtf1\\ansi{\\colortbl;"	\
				"\\red128\\green0\\blue0;"	\
				"\\red0\\green0\\blue128;"	\
				"\\red245\\green255\\blue245;"	\
				"\\red245\\green245\\blue255;"	\
				"\\red128\\green128\\blue128;"	\
				"\\red255\\green235\\blue235;"	\
			"}"
#define RTF_FOOTER			"}"
#define RTF_BEGINTAG		"\\pard "
#define RTF_INDENT_FMT		"\\fi-100\\li%d "
#define RTF_ENDTAG			"\\par"
#define RTF_BEGINTAGNAME	"\\cf1\\b "
#define RTF_ENDTAGNAME		"\\cf0\\b0 "
#define RTF_BEGINATTRNAME	"\\cf2\\b "
#define RTF_ENDATTRNAME		"\\cf0\\b0 "
#define RTF_BEGINATTRVAL	"\\b0 "
#define RTF_ENDATTRVAL		""
#define RTF_BEGINTEXT		"\\pard "
#define RTF_TEXTINDENT_FMT	"\\fi0\\li%d "
#define RTF_ENDTEXT			"\\par"
#define RTF_BEGINPLAINXML	"\\pard\\fi0\\li100\\highlight6\\cf0 "
#define RTF_ENDPLAINXML		"\\par"
#define RTF_SEPARATOR		"\\sl-1\\slmult0\\highlight5\\cf5\\-\\par\\sl0"

static void sttRtfAppendXml(StringBuf *buf, XmlNode *node, DWORD flags, int indent);

static HWND hwndJabberConsole = NULL;
static HANDLE hThreadConsole = NULL;

void JabberConsoleProcessXml(XmlNode *node, DWORD flags)
{
	if (node && hwndJabberConsole)
	{
		if (node->name)
		{
			StringBuf buf = {0};
			sttAppendBufRaw(&buf, RTF_HEADER);
			sttRtfAppendXml(&buf, node, flags, 1);
			sttAppendBufRaw(&buf, RTF_SEPARATOR);
			sttAppendBufRaw(&buf, RTF_FOOTER);
			SendMessage(hwndJabberConsole, WM_JABBER_REFRESH, 0, (LPARAM)&buf);
			sttEmptyBuf(&buf);
		} else
		{
			for (int i = 0; i < node->numChild; ++i)
				JabberConsoleProcessXml(node->child[i], flags);
		}
	}
}

static void sttAppendBufRaw(StringBuf *buf, char *str)
{
	if (!str) return;

	int length = lstrlenA(str);
	if (buf->size - buf->offset < length+1)
	{
		buf->size += (length + STRINGBUF_INCREMENT);
		buf->buf = (char *)mir_realloc(buf->buf, buf->size);
	}
	lstrcpyA(buf->buf + buf->offset, str);
	buf->offset += length;
}

static void sttAppendBufA(StringBuf *buf, char *str)
{
	char tmp[32];

	if (!str) return;

	for (char *p = str; *p; ++p)
	{
		if ((*p == '\\') || (*p == '{') || (*p == '}'))
		{
			tmp[0] = '\\';
			tmp[1] = (char)*p;
			tmp[2] = 0;
		} else
		{
			tmp[0] = (char)*p;
			tmp[1] = 0;
		}
		sttAppendBufRaw(buf, tmp);
	}
}

#ifdef UNICODE
static void sttAppendBufW(StringBuf *buf, WCHAR *str)
{
	char tmp[32];

	if (!str) return;

	sttAppendBufRaw(buf, "{\\uc1 ");
	for (WCHAR *p = str; *p; ++p)
	{
		if ((*p == '\\') || (*p == '{') || (*p == '}'))
		{
			tmp[0] = '\\';
			tmp[1] = (char)*p;
			tmp[2] = 0;
		} else
		if (*p < 128)
		{
			tmp[0] = (char)*p;
			tmp[1] = 0;
		} else
		{
			mir_snprintf(tmp, sizeof(tmp), "\\u%d ?", (int)*p);
		}
		sttAppendBufRaw(buf, tmp);
	}
	sttAppendBufRaw(buf, "}");
}
#endif

static void sttEmptyBuf(StringBuf *buf)
{
	if (buf->buf) mir_free(buf->buf);
	buf->buf = 0;
	buf->size = 0;
	buf->offset = 0;
}

static void sttRtfAppendXml(StringBuf *buf, XmlNode *node, DWORD flags, int indent)
{
	int i;
	char *indentLevel = (char *)mir_alloc(128);
	mir_snprintf(indentLevel, 128, RTF_INDENT_FMT,
		(int)(indent*200)
		);

	sttAppendBufRaw(buf, RTF_BEGINTAG);
	sttAppendBufRaw(buf, indentLevel);
	if (flags&JCPF_IN)	sttAppendBufRaw(buf, "\\highlight3 ");
	if (flags&JCPF_OUT)	sttAppendBufRaw(buf, "\\highlight4 ");
	sttAppendBufRaw(buf, "<");
	sttAppendBufRaw(buf, RTF_BEGINTAGNAME);
	sttAppendBufA(buf, node->name);
	sttAppendBufRaw(buf, RTF_ENDTAGNAME);

	for (i = 0; i < node->numAttr; ++i)
	{
		sttAppendBufRaw(buf, " ");
		sttAppendBufRaw(buf, RTF_BEGINATTRNAME);
		sttAppendBufA(buf, node->attr[i]->name);
		sttAppendBufRaw(buf, RTF_ENDATTRNAME);
		sttAppendBufRaw(buf, "=\"");
		sttAppendBufRaw(buf, RTF_BEGINATTRVAL);
		if (node->attr[i]->value)
		{
			if (flags & JCPF_UTF8)
			{
				wchar_t *tmp = mir_utf8decodeW(node->attr[i]->sendValue);
#if defined( _UNICODE )
				sttAppendBufW(buf, tmp);
#else
				char *szTmp2 = mir_u2a(tmp);
				sttAppendBufA(buf, szTmp2);
				mir_free(szTmp2);
#endif
				mir_free(tmp);
			} else
			{
				sttAppendBufT(buf, node->attr[i]->value);
			}
		}
		sttAppendBufRaw(buf, "\"");
		sttAppendBufRaw(buf, RTF_ENDATTRVAL);
	}

	if (node->numChild || node->text)
	{
		sttAppendBufRaw(buf, ">");
		sttAppendBufRaw(buf, RTF_ENDTAG);
	}

	if (node->text)
	{
		sttAppendBufRaw(buf, RTF_BEGINTEXT);

		char *indentTextLevel = (char *)mir_alloc(128);
		mir_snprintf(indentTextLevel, 128, RTF_TEXTINDENT_FMT,
			(int)((indent+1)*200)
			);
		sttAppendBufRaw(buf, indentTextLevel);
		mir_free(indentTextLevel);

		if (node->text)
		{
			if (flags & JCPF_UTF8)
			{
				wchar_t *tmp = mir_utf8decodeW(node->sendText);
#if defined( _UNICODE )
				sttAppendBufW(buf, tmp);
#else
				char *szTmp2 = mir_u2a(tmp);
				sttAppendBufA(buf, szTmp2);
				mir_free(szTmp2);
#endif
				mir_free(tmp);
			} else
			{
				sttAppendBufT(buf, node->text);
			}
		}
		sttAppendBufRaw(buf, RTF_ENDTEXT);
	}

	for (i = 0; i < node->numChild; ++i)
		sttRtfAppendXml(buf, node->child[i], flags & ~(JCPF_IN|JCPF_OUT), indent+1);

	if (node->numChild || node->text)
	{
		sttAppendBufRaw(buf, RTF_BEGINTAG);
		sttAppendBufRaw(buf, indentLevel);
		sttAppendBufRaw(buf, "</");
		sttAppendBufRaw(buf, RTF_BEGINTAGNAME);
		sttAppendBufA(buf, node->name);
		sttAppendBufRaw(buf, RTF_ENDTAGNAME);
		sttAppendBufRaw(buf, ">");
	} else
	{
		sttAppendBufRaw(buf, " />");
	}

	sttAppendBufRaw(buf, RTF_ENDTAG);
	mir_free(indentLevel);
}

DWORD CALLBACK sttStreamInCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	StringBuf *buf = (StringBuf *)dwCookie;
	*pcb = 0;

	if (buf->streamOffset < buf->offset)
	{
		*pcb = min(cb, buf->offset - buf->streamOffset);
		memcpy(pbBuff, buf->buf + buf->streamOffset, *pcb);
		buf->streamOffset += *pcb;
	}

	return 0;
}

static int JabberConsoleDlgResizer(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	switch ( urc->wId )
	{
		case IDC_CONSOLE:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
		case IDC_CONSOLEIN:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
}

static void JabberConsoleXmlCallback(XmlNode *node, void *userdata)
{
	JabberConsoleProcessXml(node, JCPF_OUT);
}

static BOOL CALLBACK JabberConsoleDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIconEx("xmlconsole"));
			SendDlgItemMessage(hwndDlg, IDC_CONSOLE, EM_SETEDITSTYLE, SES_EXTENDBACKCOLOR, SES_EXTENDBACKCOLOR);
			SendDlgItemMessage(hwndDlg, IDC_CONSOLE, EM_EXLIMITTEXT, 0, 0x80000000);
			Utils_RestoreWindowPosition(hwndDlg, NULL, jabberProtoName, "consoleWnd_");
			return TRUE;
		}

		case WM_JABBER_REFRESH:
		{
			SendDlgItemMessage(hwndDlg, IDC_CONSOLE, WM_SETREDRAW, FALSE, 0);

			StringBuf *buf = (StringBuf *)lParam;
			buf->streamOffset = 0;

			EDITSTREAM es = {0};
			es.dwCookie = (DWORD)buf;
			es.pfnCallback = sttStreamInCallback;

			SCROLLINFO si = {0};
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(GetDlgItem(hwndDlg, IDC_CONSOLE), SB_VERT, &si);

			CHARRANGE oldSel, sel;
			POINT ptScroll;
			SendDlgItemMessage(hwndDlg, IDC_CONSOLE, EM_GETSCROLLPOS, 0, (LPARAM)&ptScroll);
			SendDlgItemMessage(hwndDlg, IDC_CONSOLE, EM_EXGETSEL, 0, (LPARAM)&oldSel);
			sel.cpMin = sel.cpMax = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CONSOLE));
			SendDlgItemMessage(hwndDlg, IDC_CONSOLE, EM_EXSETSEL, 0, (LPARAM)&sel);
			SendDlgItemMessage(hwndDlg, IDC_CONSOLE, EM_STREAMIN, SF_RTF|SFF_SELECTION, (LPARAM)&es);
			SendDlgItemMessage(hwndDlg, IDC_CONSOLE, EM_EXSETSEL, 0, (LPARAM)&oldSel);

			// magic expression from tabsrmm :)
			if ((UINT)si.nPos >= (UINT)si.nMax-si.nPage-5 || si.nMax-si.nMin-si.nPage < 50)
			{
				SendDlgItemMessage(hwndDlg, IDC_CONSOLE, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
				sel.cpMin = sel.cpMax = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CONSOLE));
				SendDlgItemMessage(hwndDlg, IDC_CONSOLE, EM_EXSETSEL, 0, (LPARAM)&sel);
			} else
			{
				SendDlgItemMessage(hwndDlg, IDC_CONSOLE, EM_SETSCROLLPOS, 0, (LPARAM)&ptScroll);
			}

			SendDlgItemMessage(hwndDlg, IDC_CONSOLE, WM_SETREDRAW, TRUE, 0);
			InvalidateRect(GetDlgItem(hwndDlg, IDC_CONSOLE), NULL, FALSE);

			break;
		}

		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = 300;
			lpmmi->ptMinTrackSize.y = 200;
			return 0;
		}

		case WM_SIZE:
		{
			UTILRESIZEDIALOG urd;
			urd.cbSize = sizeof(urd);
			urd.hwndDlg = hwndDlg;
			urd.hInstance = hInst;
			urd.lpTemplate = MAKEINTRESOURCEA(IDD_CONSOLE);
			urd.lParam = 0;
			urd.pfnResizer = JabberConsoleDlgResizer;
			CallService( MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd );
			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					if (!jabberOnline)
					{
						MessageBox(hwndDlg, TranslateT("Can't send data while you are offline."), TranslateT("Jabber Error"), MB_ICONSTOP|MB_OK);
						break;
					}

					int length = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CONSOLEIN)) + 1;
					TCHAR *textToSend = (TCHAR *)mir_alloc(length * sizeof(TCHAR));
					GetWindowText(GetDlgItem(hwndDlg, IDC_CONSOLEIN), textToSend, length);
					char *tmp = mir_utf8encodeT(textToSend);
					jabberThreadInfo->send(tmp, lstrlenA(tmp));

					StringBuf buf = {0};
					sttAppendBufRaw(&buf, RTF_HEADER);
					sttAppendBufRaw(&buf, RTF_BEGINPLAINXML);
					sttAppendBufT(&buf, textToSend);
					sttAppendBufRaw(&buf, RTF_ENDPLAINXML);
					sttAppendBufRaw(&buf, RTF_SEPARATOR);
					sttAppendBufRaw(&buf, RTF_FOOTER);
					SendMessage(hwndDlg, WM_JABBER_REFRESH, 0, (LPARAM)&buf);
					sttEmptyBuf(&buf);

					XmlState xmlstate;
					JabberXmlInitState(&xmlstate);
					JabberXmlSetCallback(&xmlstate, 1, ELEM_CLOSE, JabberConsoleXmlCallback, NULL);
					JabberXmlParse(&xmlstate,tmp);
					xmlstate=xmlstate;
					JabberXmlDestroyState(&xmlstate);

					mir_free(tmp);
					mir_free(textToSend);

					SendDlgItemMessage(hwndDlg, IDC_CONSOLEIN, WM_SETTEXT, 0, (LPARAM)_T(""));
					break;
				}
				case IDCANCEL:
				{
					PostMessage(hwndDlg, WM_CLOSE, 0, 0);
					break;
				}
				case IDC_RESET:
				{
					SetDlgItemText(hwndDlg, IDC_CONSOLE, _T(""));
					break;
				}
			}
			break;
		}

		case WM_CLOSE:
			Utils_SaveWindowPosition(hwndDlg, NULL, jabberProtoName, "consoleWnd_");
			DestroyWindow(hwndDlg);
			return TRUE;

		case WM_DESTROY:
			hwndJabberConsole = NULL;
			break;
	}

	return FALSE;
}

static void __cdecl sttJabberConsoleThread(void *)
{
	MSG msg;
	while (1)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				goto LBL_Quit;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			SleepEx(1, TRUE);
		}
		SleepEx(1,TRUE);
	}

LBL_Quit:
	;
}

static void CALLBACK sttCreateConsoleAPC(DWORD dwParam)
{
	hwndJabberConsole = CreateDialog(hInst, MAKEINTRESOURCE(IDD_CONSOLE), NULL, JabberConsoleDlgProc);
	ShowWindow(hwndJabberConsole, SW_SHOW);
}

static void CALLBACK sttTerminateConsoleAPC(DWORD dwParam)
{
	PostQuitMessage(0);
}

void JabberConsoleInit()
{
	LoadLibraryA("riched20.dll");
	hThreadConsole = (HANDLE)mir_forkthread(sttJabberConsoleThread, NULL);
}

void JabberConsoleUninit()
{
	if (!hThreadConsole) return;
	QueueUserAPC(sttTerminateConsoleAPC, hThreadConsole, NULL);
	WaitForSingleObject(hThreadConsole, INFINITE);
}

int JabberMenuHandleConsole(WPARAM wParam, LPARAM lParam)
{
	if (hwndJabberConsole && IsWindow(hwndJabberConsole))
	{
		SetForegroundWindow(hwndJabberConsole);
	} else
	if (hThreadConsole)
	{
		QueueUserAPC(sttCreateConsoleAPC, hThreadConsole, NULL);
	}
	return 0;
}
