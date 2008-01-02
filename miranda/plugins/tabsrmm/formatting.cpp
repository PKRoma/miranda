/*
Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project,
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
---------------------------------------------------------------------------

$Id$

License: GPL
*/

#define __TSR_CXX
#include "commonheaders.h"

#include <tchar.h>
#include <windows.h>
#include <richedit.h>

#include <string>
#include "msgdlgutils.h"
#include "m_smileyadd.h"

#include "m_MathModule.h"

#define MWF_LOG_TEXTFORMAT 0x2000000
#define MSGDLGFONTCOUNT 22

extern "C" RTFColorTable *rtf_ctable;
extern "C" int _DebugPopup(HANDLE hContact, const char *fmt, ...);
extern "C" char *xStatusDescr[];
extern "C" TCHAR *MY_DBGetContactSettingString(HANDLE hContact, char *szModule, char *szSetting);
extern "C" DWORD m_LangPackCP;
extern "C" int MY_CallService(const char *svc, WPARAM wParam, LPARAM lParam);
extern "C" int MY_ServiceExists(const char *svc);
extern "C" void RTF_ColorAdd(const TCHAR *tszColname, size_t length);
extern "C" int  haveMathMod;
extern "C" TCHAR *mathModDelimiter;

static int iHaveSmileyadd = -1;
extern "C" unsigned int g_ctable_size;

#if defined(UNICODE)

/*
 * old code (textformat plugin dealing directly in the edit control - not the best solution, but the author
 * had no other choice as srmm never had an api for this...
 */

static WCHAR *w_bbcodes_begin[] = { L"[b]", L"[i]", L"[u]", L"[s]", L"[color=" };
static WCHAR *w_bbcodes_end[] = { L"[/b]", L"[/i]", L"[/u]", L"[/s]", L"[/color]" };

static WCHAR *formatting_strings_begin[] = { L"b1 ", L"i1 ", L"u1 ", L"s1 ", L"c1 " };
static WCHAR *formatting_strings_end[] = { L"b0 ", L"i0 ", L"u0 ", L"s0 ", L"c0 " };

#define NR_CODES 5
/*
 * this translates formatting tags into rtf sequences...
 * flags: loword = words only for simple  * /_ formatting
 *        hiword = bbcode support (strip bbcodes if 0)
 */

extern "C" const WCHAR *FilterEventMarkers(WCHAR *wszText)
{
	std::wstring text(wszText);
	unsigned int beginmark = 0, endmark = 0;

	while (TRUE) {
		if ((beginmark = text.find(_T("~-+"))) != text.npos) {
			endmark = text.find(_T("+-~"), beginmark);
			if (endmark != text.npos && (endmark - beginmark) > 5) {
				text.erase(beginmark, (endmark - beginmark) + 3);
				continue;
			}
			else
				break;
		}
		else
			break;
	}
	lstrcpyW(wszText, text.c_str());
	return wszText;
}
extern "C" const WCHAR *FormatRaw(DWORD dwFlags, const WCHAR *msg, int flags, const char *szProto, HANDLE hContact, BOOL *clr_added)
{
	bool clr_was_added = false, was_added;
	static std::wstring message(msg);
	unsigned beginmark = 0, endmark = 0, tempmark = 0, index;
	int i, endindex;
	WCHAR endmarker;
	message.assign(msg);


	if (haveMathMod && mathModDelimiter && message.find(mathModDelimiter) != message.npos)
		return(message.c_str());

	if (HIWORD(flags) == 0)
		goto nobbcode;

	if (iHaveSmileyadd == -1)
		iHaveSmileyadd = MY_ServiceExists(MS_SMILEYADD_BATCHPARSE);

	if (haveMathMod && mathModDelimiter) {
		unsigned mark = 0;
		int      nrDelims = 0;
		while ((mark = message.find(mathModDelimiter, mark)) != message.npos) {
			nrDelims++;
			mark += lstrlen(mathModDelimiter);
		}
		if (nrDelims > 0 && (nrDelims % 2) != 0)
			message.append(mathModDelimiter);
	}
	beginmark = 0;
	while (TRUE) {
		for (i = 0; i < NR_CODES; i++) {
			if ((tempmark = message.find(w_bbcodes_begin[i], 0)) != message.npos)
				break;
		}
		if (i >= NR_CODES)
			break;
		beginmark = tempmark;
		endindex = i;
		endmark = message.find(w_bbcodes_end[i], beginmark);
		if (endindex == 4) {                                 // color
			size_t closing = message.find_first_of(L"]", beginmark);
			was_added = false;

			if (closing == message.npos) {                      // must be an invalid [color=] tag w/o closing bracket
				message[beginmark] = ' ';
				continue;
			}
			else {
				std::wstring colorname = message.substr(beginmark + 7, 8);
search_again:
				bool  clr_found = false;
				unsigned int ii = 0;
				wchar_t szTemp[5];
				for (ii = 0; ii < g_ctable_size; ii++) {
					if (!_wcsnicmp((wchar_t *)colorname.c_str(), rtf_ctable[ii].szName, lstrlen(rtf_ctable[ii].szName))) {
						closing = beginmark + 7 + wcslen(rtf_ctable[ii].szName);
						if (endmark != message.npos) {
							message.erase(endmark, 4);
							message.replace(endmark, 4, L"c0 ");
						}
						message.erase(beginmark, (closing - beginmark));
						message.insert(beginmark, L"cxxx ");
						_snwprintf(szTemp, 4, L"%02d", MSGDLGFONTCOUNT + 10 + ii);
						message[beginmark + 3] = szTemp[0];
						message[beginmark + 4] = szTemp[1];
						clr_found = true;
						if (was_added) {
							wchar_t wszTemp[100];
							_snwprintf(wszTemp, 100, L"##col##%06u:%04u", endmark - closing, ii);
							wszTemp[99] = 0;
							message.insert(beginmark, wszTemp);
						}
						break;
					}
				}
				if (!clr_found) {
					size_t  c_closing = colorname.find_first_of(L"]", 0);
					if (c_closing == colorname.npos)
						c_closing = colorname.length();
					const wchar_t *wszColname = colorname.c_str();
					if (endmark != message.npos && c_closing > 2 && c_closing <= 6 && iswalnum(colorname[0]) && iswalnum(colorname[c_closing -1])) {
						RTF_ColorAdd(wszColname, c_closing);
						if (!was_added) {
							clr_was_added = was_added = true;
							goto search_again;
						}
						else
							goto invalid_code;
					}
					else {
invalid_code:
						if (endmark != message.npos)
							message.erase(endmark, 8);
						if (closing != message.npos && closing < endmark)
							message.erase(beginmark, (closing - beginmark) + 1);
						else
							message[beginmark] = ' ';
					}
				}
				continue;
			}
		}
		if (endmark != message.npos)
			message.replace(endmark, 4, formatting_strings_end[i]);
		message.insert(beginmark, L" ");
		message.replace(beginmark, 4, formatting_strings_begin[i]);
	}
nobbcode:
	if (message.find(L"://") != message.npos)
		goto nosimpletags;

	if (!(dwFlags & MWF_LOG_TEXTFORMAT))
		goto nosimpletags;

	while ((beginmark = message.find_first_of(L"*/_", beginmark)) != message.npos) {
		endmarker = message[beginmark];
		if (LOWORD(flags)) {
			if (beginmark > 0 && !iswspace(message[beginmark - 1]) && !iswpunct(message[beginmark - 1])) {
				beginmark++;
				continue;
			}
			// search a corresponding endmarker which fulfills the criteria
			unsigned tempmark = beginmark + 1;
			while ((endmark = message.find(endmarker, tempmark)) != message.npos) {
				if (iswpunct(message[endmark + 1]) || iswspace(message[endmark + 1]) || message[endmark + 1] == 0 || wcschr(L"*/_", message[endmark + 1]) != NULL)
					goto ok;
				tempmark = endmark + 1;
			}
			break;
		}
		else {
			if ((endmark = message.find(endmarker, beginmark + 1)) == message.npos)
				break;
		}
ok:
		if ((endmark - beginmark) < 2) {
			beginmark++;
			continue;
		}
		index = 0;
		switch (endmarker) {
			case '*':
				index = 0;
				break;
			case '/':
				index = 1;
				break;
			case '_':
				index = 2;
				break;
		}

		/*
		 * check if the code enclosed by simple formatting tags is a valid smiley code and skip formatting if
		 * it really is one.
		 */

		if (iHaveSmileyadd && endmark > beginmark + 1) {
			std::wstring smcode;
			smcode.assign(message, beginmark, (endmark - beginmark) + 1);
			SMADD_BATCHPARSE2 smbp = {0};
			SMADD_BATCHPARSERES *smbpr;

			smbp.cbSize = sizeof(smbp);
			smbp.Protocolname = szProto;
			smbp.flag = SAFL_TCHAR | SAFL_PATH;
			smbp.str = (TCHAR *)smcode.c_str();
			smbp.hContact = hContact;
			smbpr = (SMADD_BATCHPARSERES *)MY_CallService(MS_SMILEYADD_BATCHPARSE, 0, (LPARAM)&smbp);
			if (smbpr) {
				MY_CallService(MS_SMILEYADD_BATCHFREE, 0, (LPARAM)smbpr);
				beginmark = endmark + 1;
				continue;
			}
		}
		message.insert(endmark, L"%%%");
		message.replace(endmark, 4, formatting_strings_end[index]);
		message.insert(beginmark, L"%%%");
		message.replace(beginmark, 4, formatting_strings_begin[index]);
	}
nosimpletags:
	if (clr_added && clr_was_added)
		*clr_added = TRUE;
	return(message.c_str());
}

#else   // unicode

/* this is the ansi version */
static char *bbcodes_begin[] = { "[b]", "[i]", "[u]", "[s]", "[color=" };
static char *bbcodes_end[] = { "[/b]", "[/i]", "[/u]", "[/s]", "[/color]" };

#define NR_CODES 5

static char *formatting_strings_begin[] = { "b1 ", "i1 ", "u1 ", "s1 ", "c1 " };
static char *formatting_strings_end[] = { "b0 ", "i0 ", "u0 ", "s0 ", "c0 " };

/*
* this translates formatting tags into rtf sequences...
*/

extern "C" const char *FormatRaw(DWORD dwFlags, const char *msg, int flags, const char *szProto, HANDLE hContact, BOOL *clr_added)
{
	bool clr_was_added = false, was_added;
	static std::string message(msg);
	unsigned beginmark = 0, endmark = 0, tempmark = 0, index;
	char endmarker;
	int i, endindex;
	message.assign(msg);

	if (haveMathMod && mathModDelimiter && message.find(mathModDelimiter) != message.npos)
		return(message.c_str());

	if (HIWORD(flags) == 0)
		goto nobbcode;

	if (iHaveSmileyadd == -1)
		iHaveSmileyadd = MY_ServiceExists(MS_SMILEYADD_BATCHPARSE);

	if (haveMathMod && mathModDelimiter) {
		unsigned mark = 0;
		int      nrDelims = 0;
		while ((mark = message.find(mathModDelimiter, mark)) != message.npos) {
			nrDelims++;
			mark += lstrlen(mathModDelimiter);
		}
		if (nrDelims > 0 && (nrDelims % 2) != 0)
			message.append(mathModDelimiter);
	}

	while (TRUE) {
		for (i = 0; i < NR_CODES; i++) {
			if ((tempmark = message.find(bbcodes_begin[i], 0)) != message.npos)
				break;
		}
		if (i >= NR_CODES)
			break;

		beginmark = tempmark;
		endindex = i;
		endmark = message.find(bbcodes_end[i], beginmark);
		if (endindex == 4) {                                 // color
			size_t closing = message.find_first_of("]", beginmark);
			was_added = false;

			if (closing == message.npos) {                      // must be an invalid [color=] tag w/o closing bracket
				message[beginmark] = ' ';
				continue;
			}
			else {
				std::string colorname = message.substr(beginmark + 7, 8);
search_again:
				bool  clr_found = false;
				unsigned int ii = 0;
				char szTemp[5];
				for (ii = 0; ii < g_ctable_size; ii++) {
					if (!_strnicmp((char *)colorname.c_str(), rtf_ctable[ii].szName, lstrlenA(rtf_ctable[ii].szName))) {
						closing = beginmark + 7 + lstrlenA(rtf_ctable[ii].szName);
						if (endmark != message.npos) {
							message.erase(endmark, 8);
							message.insert(endmark, "c0xx ");
						}
						message.erase(beginmark, (closing - beginmark) + 1);
						message.insert(beginmark, "cxxx ");
						_snprintf(szTemp, 4, "%02d", MSGDLGFONTCOUNT + 10 + ii);
						message[beginmark + 3] = szTemp[0];
						message[beginmark + 4] = szTemp[1];
						clr_found = true;
						if (was_added) {
							char wszTemp[100];
							_snprintf(wszTemp, 100, "##col##%06u:%04u", endmark - closing, ii);
							wszTemp[99] = 0;
							message.insert(beginmark, wszTemp);
						}
						break;
					}
				}
				if (!clr_found) {
					size_t  c_closing = colorname.find_first_of("]", 0);
					if (c_closing == colorname.npos)
						c_closing = colorname.length();
					const char *wszColname = colorname.c_str();
					if (endmark != message.npos && c_closing > 2 && c_closing <= 6 && isalnum(colorname[0]) && isalnum(colorname[c_closing -1])) {
						RTF_ColorAdd(wszColname, c_closing);
						if (!was_added) {
							clr_was_added = was_added = true;
							goto search_again;
						}
						else
							goto invalid_code;
					}
					else {
invalid_code:
						if (endmark != message.npos)
							message.erase(endmark, 8);
						if (closing != message.npos && closing < endmark)
							message.erase(beginmark, (closing - beginmark) + 1);
						else
							message[beginmark] = ' ';
					}
				}
				continue;
			}
		}
		if (endmark != message.npos)
			message.replace(endmark, 4, formatting_strings_end[i]);
		message.insert(beginmark, " ");
		message.replace(beginmark, 4, formatting_strings_begin[i]);
	}

nobbcode:
	if (message.find("://") != message.npos)
		goto nosimpletags;

	if (!(dwFlags & MWF_LOG_TEXTFORMAT))           // skip */_ stuff if not enabled
		goto nosimpletags;

	while ((beginmark = message.find_first_of("*/_", beginmark)) != message.npos) {
		endmarker = message[beginmark];
		if (LOWORD(flags)) {
			if (beginmark > 0 && !isspace(message[beginmark - 1]) && !ispunct(message[beginmark - 1])) {
				beginmark++;
				continue;
			}
			// search a corresponding endmarker which fulfills the criteria
			unsigned tempmark = beginmark + 1;
			while ((endmark = message.find(endmarker, tempmark)) != message.npos) {
				if (ispunct(message[endmark + 1]) || isspace(message[endmark + 1]) || message[endmark + 1] == 0 || strchr("*/_", message[endmark + 1]) != NULL)
					goto ok;
				tempmark = endmark + 1;
			}
			break;
		}
		else {
			if ((endmark = message.find(endmarker, beginmark + 1)) == message.npos)
				break;
		}
ok:
		if ((endmark - beginmark) < 2) {
			beginmark++;
			continue;
		}
		index = 0;
		switch (endmarker) {
			case '*':
				index = 0;
				break;
			case '/':
				index = 1;
				break;
			case '_':
				index = 2;
				break;
		}
		if (iHaveSmileyadd && endmark > beginmark + 1) {
			std::string smcode;
			smcode.assign(message, beginmark, (endmark - beginmark) + 1);
			SMADD_BATCHPARSE2 smbp = {0};
			SMADD_BATCHPARSERES *smbpr;

			smbp.cbSize = sizeof(smbp);
			smbp.Protocolname = szProto;
			smbp.flag = SAFL_TCHAR | SAFL_PATH;
			smbp.str = (TCHAR *)smcode.c_str();
			smbp.hContact = hContact;
			smbpr = (SMADD_BATCHPARSERES *)MY_CallService(MS_SMILEYADD_BATCHPARSE, 0, (LPARAM)&smbp);
			if (smbpr) {
				MY_CallService(MS_SMILEYADD_BATCHFREE, 0, (LPARAM)smbpr);
				beginmark = endmark + 1;
				continue;
			}
		}
		message.insert(endmark, "%%%");
		message.replace(endmark, 4, formatting_strings_end[index]);
		message.insert(beginmark, "%%%");
		message.replace(beginmark, 4, formatting_strings_begin[index]);
	}
nosimpletags:
	if (clr_added && clr_was_added)
		*clr_added = TRUE;
	return(message.c_str());
}

#endif

/*
 * format the title bar string for IM chat sessions.
 * the caller must free() the return value
 */

#if defined(_UNICODE) || defined(UNICODE)

static TCHAR *title_variables[] = { _T("%n"), _T("%s"), _T("%u"), _T("%p"), _T("%c"), _T("%x"), _T("%m")};
#define NR_VARS 7		// nr. of variables we can handle

extern "C" int MY_DBGetContactSettingTString(HANDLE hContact, char *szModule, char *szSetting, DBVARIANT *dbv);
extern "C" int MY_DBFreeVariant(DBVARIANT *dbv);

extern "C" WCHAR *NewTitle(HANDLE hContact, const TCHAR *szFormat, const TCHAR *szNickname, const char *szStatus, const TCHAR *szContainer, const char *szUin, const char *szProto, DWORD idle, UINT codePage, BYTE xStatus, WORD wStatus)
{
	TCHAR *szResult = 0;
	int length = 0;
	int i, tempmark = 0;
	TCHAR szTemp[512];

	std::wstring title(szFormat);

	while (TRUE) {
		for (i = 0; i < NR_VARS; i++) {
			if ((tempmark = title.find(title_variables[i], 0)) != title.npos)
				break;
		}
		if (i >= NR_VARS)
			break;

		switch (title[tempmark + 1]) {
			case 'n': {
				if (szNickname)
					title.insert(tempmark + 2, szNickname);
				title.erase(tempmark, 2);
				break;
			}
			case 's': {
				if (szStatus && szStatus[0]) {
					MultiByteToWideChar(m_LangPackCP, 0, szStatus, -1, szTemp, 500);
					title.insert(tempmark + 2, szTemp);
				}
				title.erase(tempmark, 2);
				break;
			}
			case 'u': {
				if (szUin) {
					MultiByteToWideChar(CP_ACP, 0, szUin, -1, szTemp, 500);
					title.insert(tempmark + 2, szTemp);
				}
				title.erase(tempmark, 2);
				break;
			}
			case 'c': {
				title.insert(tempmark + 2, szContainer);
				title.erase(tempmark, 2);
				break;
			}
			case 'p': {
				if (szProto) {
					MultiByteToWideChar(CP_ACP, 0, szProto, -1, szTemp, 500);
					title.insert(tempmark + 2, szTemp);
				}
				title.erase(tempmark, 2);
				break;
			}
			case 'x': {
				char *szFinalStatus = NULL;

				if (wStatus != ID_STATUS_OFFLINE && xStatus > 0 && xStatus <= 31) {
					DBVARIANT dbv = {0};

					if (!MY_DBGetContactSettingTString(hContact, (char *)szProto, "XStatusName", &dbv)) {
						_tcsncpy(szTemp, dbv.ptszVal, 500);
						szTemp[500] = 0;
						MY_DBFreeVariant(&dbv);
					}
					else
						szFinalStatus = (char *)xStatusDescr[xStatus - 1];
					if (szFinalStatus)
						MultiByteToWideChar(m_LangPackCP, 0, szFinalStatus, -1, szTemp, 500);

					title.insert(tempmark + 2, szTemp);
				}
				title.erase(tempmark, 2);
				break;
			}
			case 'm': {
				char *szFinalStatus = NULL;

				if (wStatus != ID_STATUS_OFFLINE && xStatus > 0 && xStatus <= 31) {
					DBVARIANT dbv = {0};

					if (!MY_DBGetContactSettingTString(hContact, (char *)szProto, "XStatusName", &dbv)) {
						_tcsncpy(szTemp, dbv.ptszVal, 500);
						szTemp[500] = 0;
						MY_DBFreeVariant(&dbv);
					}
					else
						szFinalStatus = (char *)xStatusDescr[xStatus - 1];
				}
				else
					szFinalStatus = (char *)(szStatus && szStatus[0] ? szStatus : "(undef)");

				if (szFinalStatus)
					MultiByteToWideChar(m_LangPackCP, 0, szFinalStatus, -1, szTemp, 500);

				title.insert(tempmark + 2, szTemp);
				title.erase(tempmark, 2);
				break;
			}
			default:
				title.erase(tempmark, 1);
				break;
		}
	}
	length = title.length();

	szResult = (TCHAR *)malloc((length + 2) * sizeof(TCHAR));
	if (szResult) {
		_tcsncpy(szResult, title.c_str(), length);
		szResult[length] = 0;
	}
	return szResult;
}

extern "C" const WCHAR *EncodeWithNickname(const char *string, const WCHAR *szNick, UINT codePage)
{
	static std::wstring msg;
	wchar_t stringW[256];
	int mark = 0;

	MultiByteToWideChar(codePage, 0, string, -1, stringW, 256);
	stringW[255] = 0;
	msg.assign(stringW);
	if ((mark = msg.find(L"%nick%")) != msg.npos) {
		msg.erase(mark, 6);
		msg.insert(mark, szNick, lstrlenW(szNick));
	}
	return msg.c_str();
}

#else

static TCHAR *title_variables[] = { _T("%n"), _T("%s"), _T("%u"), _T("%p"), _T("%c"), _T("%x"), _T("%m")};
#define NR_VARS 7

extern "C" char *NewTitle(HANDLE hContact, const TCHAR *szFormat, const TCHAR *szNickname, const char *szStatus, const TCHAR *szContainer, const char *szUin, const char *szProto, DWORD idle, UINT codePage, BYTE xStatus, WORD wStatus)
{
	TCHAR *szResult = 0;
	int length = 0;
	int i, tempmark = 0;

	std::string title(szFormat);

	while (TRUE) {
		for (i = 0; i < NR_VARS; i++) {
			if ((tempmark = title.find(title_variables[i], 0)) != title.npos)
				break;
		}
		if (i >= NR_VARS)
			break;

		switch (title[tempmark + 1]) {
			case 'n': {
				if (szNickname)
					title.insert(tempmark + 2, szNickname);
				title.erase(tempmark, 2);
				break;
			}
			case 's': {
				if (szStatus && szStatus[0])
					title.insert(tempmark + 2, szStatus);
				title.erase(tempmark, 2);
				break;
			}
			case 'u': {
				if (szUin)
					title.insert(tempmark + 2, szUin);
				title.erase(tempmark, 2);
				break;
			}
			case 'c': {
				title.insert(tempmark + 2, szContainer);
				title.erase(tempmark, 2);
				break;
			}
			case 'p': {
				if (szProto)
					title.insert(tempmark + 2, szProto);
				title.erase(tempmark, 2);
				break;
			}
			case 'x': {
				if (wStatus != ID_STATUS_OFFLINE && xStatus > 0 && xStatus <= 31)
					title.insert(tempmark + 2, xStatusDescr[xStatus - 1]);
				title.erase(tempmark, 2);
				break;
			}
			case 'm': {
				char *szFinalStatus = NULL;
				TCHAR *result = NULL;

				if (wStatus != ID_STATUS_OFFLINE && xStatus > 0 && xStatus <= 32) {
					if ((result = MY_DBGetContactSettingString(hContact, (char *)szProto, "XStatusName")) != NULL) {
						szFinalStatus = result;
					}
					else
						szFinalStatus = (char *)(szStatus && szStatus[0] ? szStatus : "(undef)");
				}
				else
					szFinalStatus = (char *)(szStatus && szStatus[0] ? szStatus : "(undef)");

				title.insert(tempmark + 2, szFinalStatus);
				title.erase(tempmark, 2);

				if (result)
					free(result);
				break;
			}
			default:
				title.erase(tempmark, 1);
				break;
		}
	}
	length = title.length();

	szResult = (TCHAR *)malloc((title.length() + 2) * sizeof(TCHAR));
	if (szResult) {
		_tcsncpy(szResult, title.c_str(), length);
		szResult[length] = 0;
	}
	return szResult;
}

#endif

extern "C" const char *FilterEventMarkersA(char *szText)
{
	std::string text(szText);
	unsigned int beginmark = 0, endmark = 0;

	while (TRUE) {
		if ((beginmark = text.find("~-+")) != text.npos) {
			endmark = text.find("+-~", beginmark);
			if (endmark != text.npos && (endmark - beginmark) > 5) {
				text.erase(beginmark, (endmark - beginmark) + 3);
				continue;
			}
			else
				break;
		}
		else
			break;
	}
	lstrcpyA(szText, text.c_str());
	return szText;
}
