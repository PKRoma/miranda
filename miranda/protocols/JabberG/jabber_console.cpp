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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "resource.h"
#include <richedit.h>

#define JCPF_IN			0x01UL
#define JCPF_OUT		0x02UL
#define JCPF_ERROR		0x04UL

#define JCPF_UTF8		0x08UL
#define JCPF_TCHAR		0x00UL

#define WM_CREATECONSOLE  WM_USER+1000

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


struct TFilterInfo
{
	enum Type { T_JID, T_XMLNS, T_ANY, T_OFF };

	volatile BOOL msg, presence, iq;
	volatile Type type;

	CRITICAL_SECTION csPatternLock;
	TCHAR pattern[256];
};
static TFilterInfo g_filterInfo = {0};
static bool sttFilterXml(XmlNode *node, DWORD flags);

void CJabberProto::JabberConsoleProcessXml(XmlNode *node, DWORD flags)
{
	if ( node && hwndJabberConsole ) {
		if ( node->name ) {
			if ( sttFilterXml( node, flags )) {
				StringBuf buf = {0};
				sttAppendBufRaw(&buf, RTF_HEADER);
				sttRtfAppendXml(&buf, node, flags, 1);
				sttAppendBufRaw(&buf, RTF_SEPARATOR);
				sttAppendBufRaw(&buf, RTF_FOOTER);
				SendMessage(hwndJabberConsole, WM_JABBER_REFRESH, 0, (LPARAM)&buf);
				sttEmptyBuf(&buf);
			}
		}
		else {
			for ( int i = 0; i < node->numChild; ++i )
				JabberConsoleProcessXml( node->child[i], flags );
		}
	}
}

static bool sttRecursiveCheckFilter(XmlNode *node, DWORD flags)
{
	int i;

	for (i = 0; i < node->numAttr; ++i)
	{
		TCHAR *attrValue = node->attr[i]->value;
		if (flags & JCPF_UTF8)
		{
			wchar_t *tmp = mir_utf8decodeW(node->attr[i]->sendValue);
			#ifdef UNICODE
			attrValue = tmp;
			#else
			attrValue = mir_u2a(tmp);
			mir_free(tmp);
			#endif
		}

		if (JabberStrIStr(attrValue, g_filterInfo.pattern))
		{
			if (flags & JCPF_UTF8)  mir_free(attrValue);
			return true;
		}

		if (flags & JCPF_UTF8) mir_free(attrValue);
	}

	for (i = 0; i < node->numChild; ++i)
		if (sttRecursiveCheckFilter(node->child[i], flags))
			return true;

	return false;
}

static bool sttFilterXml(XmlNode *node, DWORD flags)
{
	if (!g_filterInfo.msg && !lstrcmpA(node->name, "message")) return false;
	if (!g_filterInfo.presence && !lstrcmpA(node->name, "presence")) return false;
	if (!g_filterInfo.iq && !lstrcmpA(node->name, "iq")) return false;
	if (g_filterInfo.type == TFilterInfo::T_OFF) return true;

	bool result = false;
	EnterCriticalSection(&g_filterInfo.csPatternLock);

	switch (g_filterInfo.type)
	{
		case TFilterInfo::T_JID:
		{
			TCHAR *attrValue = JabberXmlGetAttrValue(node, (flags&JCPF_OUT)?"to":"from");
			if (!attrValue) break;

			if (flags & JCPF_UTF8)
			{
				wchar_t *tmp = mir_utf8decodeW((char *)attrValue);
				#ifdef UNICODE
				attrValue = tmp;
				#else
				attrValue = mir_u2a(tmp);
				mir_free(tmp);
				#endif
			}
			result = JabberStrIStr(attrValue, g_filterInfo.pattern) ? true : false;
			if (flags & JCPF_UTF8) mir_free(attrValue);
			break;
		}
		case TFilterInfo::T_XMLNS:
		{
			if (node->numChild != 1) break;

			TCHAR *attrValue = JabberXmlGetAttrValue(node->child[0], "xmlns");
			if (!attrValue) break;

			if (flags & JCPF_UTF8)
			{
				wchar_t *tmp = mir_utf8decodeW((char *)attrValue);
				#ifdef UNICODE
				attrValue = tmp;
				#else
				attrValue = mir_u2a(tmp);
				mir_free(tmp);
				#endif
			}
			result = JabberStrIStr(attrValue, g_filterInfo.pattern) ? true : false;
			if (flags & JCPF_UTF8) mir_free(attrValue);

			break;
		}

		case TFilterInfo::T_ANY:
		{
			result = sttRecursiveCheckFilter(node, flags);
			break;
		}
	}

	LeaveCriticalSection(&g_filterInfo.csPatternLock);
	return result;
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
		if (node->numChild)
			sttAppendBufRaw(buf, RTF_ENDTAG);
	}

	if (node->text)
	{
		if (node->numChild)
		{
			sttAppendBufRaw(buf, RTF_BEGINTEXT);
			char *indentTextLevel = (char *)mir_alloc(128);
			mir_snprintf( indentTextLevel, 128, RTF_TEXTINDENT_FMT, (int)(( indent + 1) * 200 ));
			sttAppendBufRaw(buf, indentTextLevel);
			mir_free(indentTextLevel);
		}

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
		if (node->numChild)
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

		case IDC_BTN_MSG:
		case IDC_BTN_PRESENCE:
		case IDC_BTN_IQ:
		case IDC_BTN_FILTER:
			return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM;

		case IDC_CB_FILTER:
		{
			RECT rc;
			GetWindowRect(GetDlgItem(hwndDlg, urc->wId), &rc);
			urc->rcItem.right += (urc->dlgNewSize.cx - urc->dlgOriginalSize.cx);
			urc->rcItem.top += (urc->dlgNewSize.cy - urc->dlgOriginalSize.cy);
			urc->rcItem.bottom = urc->rcItem.top + rc.bottom - rc.top;
			return 0;
		}
	}
	return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
}

static void JabberConsoleXmlCallback(XmlNode *node, CJabberProto *ppro)
{
	ppro->JabberConsoleProcessXml(node, JCPF_OUT);
}

static void sttJabberConsoleRebuildStrings(CJabberProto* ppro, HWND hwndCombo)
{
	int i;
	JABBER_LIST_ITEM *item = NULL;

	int len = GetWindowTextLength(hwndCombo) + 1;
	TCHAR *buf = (TCHAR *)_alloca(len * sizeof(TCHAR));
	GetWindowText(hwndCombo, buf, len);

	SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);

	for (i = 0; g_JabberFeatCapPairs[i].szFeature; ++i)
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)g_JabberFeatCapPairs[i].szFeature);
	for (i = 0; g_JabberFeatCapPairsExt[i].szFeature; ++i)
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)g_JabberFeatCapPairsExt[i].szFeature);

	for (i = 0; i >= 0; i = ppro->JabberListFindNext(LIST_ROSTER, i+1))
		if (item = ppro->JabberListGetItemPtrFromIndex(i))
			SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)item->jid);
	for (i = 0; i >= 0; i = ppro->JabberListFindNext(LIST_CHATROOM, i+1))
		if (item = ppro->JabberListGetItemPtrFromIndex(i))
			SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)item->jid);

	SetWindowText(hwndCombo, buf);
}

static BOOL CALLBACK JabberConsoleDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static struct
	{
		int type;
		TCHAR *title;
		char *icon;
	} filter_modes[] =
	{
		{TFilterInfo::T_JID,	_T("JID"),				"main"},
		{TFilterInfo::T_XMLNS,	_T("xmlns"),			"xmlconsole"},
		{TFilterInfo::T_ANY,	_T("all attributes"),	"sd_filter_apply"},
		{TFilterInfo::T_OFF,	_T("disabled"),			"sd_filter_reset"},
	};

	CJabberProto* ppro = (CJabberProto*)GetWindowLong( hwndDlg, GWL_USERDATA );

	switch ( msg )
	{
		case WM_INITDIALOG:
		{
			int i;
			ppro = (CJabberProto*)lParam;
			SetWindowLong( hwndDlg, GWL_USERDATA, (LPARAM)ppro );

			TranslateDialogDefault(hwndDlg);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)ppro->LoadIconEx("xmlconsole"));
			SendDlgItemMessage(hwndDlg, IDC_CONSOLE, EM_SETEDITSTYLE, SES_EXTENDBACKCOLOR, SES_EXTENDBACKCOLOR);
			SendDlgItemMessage(hwndDlg, IDC_CONSOLE, EM_EXLIMITTEXT, 0, 0x80000000);

			g_filterInfo.msg = DBGetContactSettingByte(NULL, ppro->szProtoName, "consoleWnd_msg", TRUE);
			g_filterInfo.presence = DBGetContactSettingByte(NULL, ppro->szProtoName, "consoleWnd_presence", TRUE);
			g_filterInfo.iq = DBGetContactSettingByte(NULL, ppro->szProtoName, "consoleWnd_iq", TRUE);
			g_filterInfo.type = (TFilterInfo::Type)DBGetContactSettingByte(NULL, ppro->szProtoName, "consoleWnd_ftype", TFilterInfo::T_OFF);

			DBVARIANT dbv;
			*g_filterInfo.pattern = 0;
			if ( !ppro->JGetStringT(NULL, "consoleWnd_fpattern", &dbv))
			{
				lstrcpyn(g_filterInfo.pattern, dbv.ptszVal, SIZEOF(g_filterInfo.pattern));
				JFreeVariant(&dbv);
			}

			sttJabberConsoleRebuildStrings(ppro, GetDlgItem(hwndDlg, IDC_CB_FILTER));
			SetWindowText(GetDlgItem(hwndDlg, IDC_CB_FILTER), g_filterInfo.pattern);

			static struct
			{
				int idc;
				TCHAR *title;
				char *icon;
				bool push;
				BOOL pushed;
			} buttons[] =
			{
				{IDC_BTN_MSG,				_T("Messages"),		"pl_msg_allow",		true,	g_filterInfo.msg},
				{IDC_BTN_PRESENCE,			_T("Presences"),	"pl_prin_allow",	true,	g_filterInfo.presence},
				{IDC_BTN_IQ,				_T("Queries"),		"pl_iq_allow",		true,	g_filterInfo.iq},
				{IDC_BTN_FILTER,			_T("Filter mode"),	"sd_filter_apply",	true,	FALSE},
				{IDC_BTN_FILTER_REFRESH,	_T("Refresh list"),	"sd_nav_refresh",	false,	FALSE},
			};
			for (i = 0; i < SIZEOF(buttons); ++i)
			{
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BM_SETIMAGE, IMAGE_ICON, (LPARAM)ppro->LoadIconEx(buttons[i].icon));
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASFLATBTN, 0, 0);
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONADDTOOLTIP, (WPARAM)TranslateTS(buttons[i].title), BATF_TCHAR);
				if (buttons[i].push) SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASPUSHBTN, 0, 0);
				if (buttons[i].pushed) CheckDlgButton(hwndDlg, buttons[i].idc, TRUE);
			}

			for (i = 0; i < SIZEOF(filter_modes); ++i)
				if (filter_modes[i].type == g_filterInfo.type)
				{
					SendDlgItemMessage(hwndDlg, IDC_BTN_FILTER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)ppro->LoadIconEx(filter_modes[i].icon));
					break;
				}
			EnableWindow(GetDlgItem(hwndDlg, IDC_CB_FILTER), (g_filterInfo.type == TFilterInfo::T_OFF) ? FALSE : TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_FILTER_REFRESH), (g_filterInfo.type == TFilterInfo::T_OFF) ? FALSE : TRUE);

			Utils_RestoreWindowPosition(hwndDlg, NULL, ppro->szProtoName, "consoleWnd_");
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
			lpmmi->ptMinTrackSize.y = 400;
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
					if (!ppro->jabberOnline)
					{
						MessageBox(hwndDlg, TranslateT("Can't send data while you are offline."), TranslateT("Jabber Error"), MB_ICONSTOP|MB_OK);
						break;
					}

					int length = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CONSOLEIN)) + 1;
					TCHAR *textToSend = (TCHAR *)mir_alloc(length * sizeof(TCHAR));
					GetWindowText(GetDlgItem(hwndDlg, IDC_CONSOLEIN), textToSend, length);
					char *tmp = mir_utf8encodeT(textToSend);
					ppro->jabberThreadInfo->send(tmp, lstrlenA(tmp));

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
					JabberXmlSetCallback(&xmlstate, 1, ELEM_CLOSE, ( JABBER_XML_CALLBACK )JabberConsoleXmlCallback, ppro);
					ppro->JabberXmlParse(&xmlstate,tmp);
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

				case IDC_BTN_MSG:
				case IDC_BTN_PRESENCE:
				case IDC_BTN_IQ:
				{
					g_filterInfo.msg = IsDlgButtonChecked(hwndDlg, IDC_BTN_MSG);
					g_filterInfo.presence = IsDlgButtonChecked(hwndDlg, IDC_BTN_PRESENCE);
					g_filterInfo.iq = IsDlgButtonChecked(hwndDlg, IDC_BTN_IQ);
					break;
				}

				case IDC_BTN_FILTER_REFRESH:
				{
					sttJabberConsoleRebuildStrings(ppro, GetDlgItem(hwndDlg, IDC_CB_FILTER));
					break;
				}

				case IDC_BTN_FILTER:
				{
					int i;
					HMENU hMenu = CreatePopupMenu();
					for (i = 0; i < SIZEOF(filter_modes); ++i)
					{
						AppendMenu(hMenu,
							MF_STRING | ((filter_modes[i].type == g_filterInfo.type) ? MF_CHECKED : 0),
							filter_modes[i].type+1, TranslateTS(filter_modes[i].title));
					}
					RECT rc; GetWindowRect(GetDlgItem(hwndDlg, IDC_BTN_FILTER), &rc);
					CheckDlgButton(hwndDlg, IDC_BTN_FILTER, TRUE);
					int res = TrackPopupMenu(hMenu, TPM_RETURNCMD|TPM_BOTTOMALIGN, rc.left, rc.top, 0, hwndDlg, NULL);
					CheckDlgButton(hwndDlg, IDC_BTN_FILTER, FALSE);
					DestroyMenu(hMenu);

					if (res)
					{
						g_filterInfo.type = (TFilterInfo::Type)(res - 1);
						for (i = 0; i < SIZEOF(filter_modes); ++i)
							if (filter_modes[i].type == g_filterInfo.type)
							{
								SendDlgItemMessage(hwndDlg, IDC_BTN_FILTER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)ppro->LoadIconEx(filter_modes[i].icon));
								break;
							}
						EnableWindow(GetDlgItem(hwndDlg, IDC_CB_FILTER), (g_filterInfo.type == TFilterInfo::T_OFF) ? FALSE : TRUE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_FILTER_REFRESH), (g_filterInfo.type == TFilterInfo::T_OFF) ? FALSE : TRUE);
					}

					break;
				}

				case IDC_CB_FILTER:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						int idx = SendDlgItemMessage(hwndDlg, IDC_CB_FILTER, CB_GETCURSEL, 0, 0);
						int len = SendDlgItemMessage(hwndDlg, IDC_CB_FILTER, CB_GETLBTEXTLEN, idx, 0) + 1;

						EnterCriticalSection(&g_filterInfo.csPatternLock);
						if (len > SIZEOF(g_filterInfo.pattern))
						{
							TCHAR *buf = (TCHAR *)_alloca(len * sizeof(TCHAR));
							SendDlgItemMessage(hwndDlg, IDC_CB_FILTER, CB_GETLBTEXT, idx, (LPARAM)buf);
							lstrcpyn(g_filterInfo.pattern, buf, SIZEOF(g_filterInfo.pattern));
						} else
						{
							SendDlgItemMessage(hwndDlg, IDC_CB_FILTER, CB_GETLBTEXT, idx, (LPARAM)g_filterInfo.pattern);
						}
						LeaveCriticalSection(&g_filterInfo.csPatternLock);
					} else
					if (HIWORD(wParam) == CBN_EDITCHANGE)
					{
						EnterCriticalSection(&g_filterInfo.csPatternLock);
						GetWindowText(GetDlgItem(hwndDlg, IDC_CB_FILTER), g_filterInfo.pattern, SIZEOF(g_filterInfo.pattern));
						LeaveCriticalSection(&g_filterInfo.csPatternLock);
					}
					break;
				}
			}
			break;
		}

		case WM_CLOSE:
			DBWriteContactSettingByte(NULL, ppro->szProtoName, "consoleWnd_msg", g_filterInfo.msg);
			DBWriteContactSettingByte(NULL, ppro->szProtoName, "consoleWnd_presence", g_filterInfo.presence);
			DBWriteContactSettingByte(NULL, ppro->szProtoName, "consoleWnd_iq", g_filterInfo.iq);
			DBWriteContactSettingByte(NULL, ppro->szProtoName, "consoleWnd_ftype", g_filterInfo.type);
			ppro->JSetStringT(NULL, "consoleWnd_fpattern", g_filterInfo.pattern);

			Utils_SaveWindowPosition(hwndDlg, NULL, ppro->szProtoName, "consoleWnd_");
			DestroyWindow(hwndDlg);
			return TRUE;

		case WM_DESTROY:
			ppro->hwndJabberConsole = NULL;
			break;
	}

	return FALSE;
}

static UINT WINAPI sttJabberConsoleThread( void* param )
{
	CJabberProto* ppro = (CJabberProto*)param;
	MSG msg;
	while ( GetMessage(&msg, NULL, 0, 0 )) {
		if ( msg.message == WM_CREATECONSOLE ) {
			ppro->hwndJabberConsole = CreateDialogParam( hInst, MAKEINTRESOURCE(IDD_CONSOLE), NULL, JabberConsoleDlgProc, (LPARAM)ppro );
			ShowWindow( ppro->hwndJabberConsole, SW_SHOW );
			continue;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

void CJabberProto::JabberConsoleInit()
{
	LoadLibraryA("riched20.dll");
	InitializeCriticalSection(&g_filterInfo.csPatternLock);
	hThreadConsole = (HANDLE)mir_forkthreadex( ::sttJabberConsoleThread, this, 0, &sttJabberConsoleThreadId);
}

void CJabberProto::JabberConsoleUninit()
{
	if ( hThreadConsole ) 
		PostThreadMessage(sttJabberConsoleThreadId, WM_QUIT, 0, 0);

	g_filterInfo.iq = g_filterInfo.msg = g_filterInfo.presence = FALSE;
	g_filterInfo.type = TFilterInfo::T_OFF;
	DeleteCriticalSection(&g_filterInfo.csPatternLock);
}

int JabberMenuHandleConsole(WPARAM wParam, LPARAM lParam, CJabberProto* ppro)
{
	HWND hwndJabberConsole = ppro->hwndJabberConsole;
	if (hwndJabberConsole && IsWindow(hwndJabberConsole))
		SetForegroundWindow(hwndJabberConsole);
	else
		if (ppro->hThreadConsole)
			PostThreadMessage(ppro->sttJabberConsoleThreadId, WM_CREATECONSOLE, 0, 0);
	return 0;
}
