/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2009 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * some text utility functions for message formatting (bbcode translation etc.)
 *
 */

#define __CPP_LEAN

#include "commonheaders.h"

#include <string>

#define MWF_LOG_BBCODE 1
#define MWF_LOG_TEXTFORMAT 0x2000000
#define MSGDLGFONTCOUNT 22

extern RTFColorTable *rtf_ctable;
extern void RTF_ColorAdd(const TCHAR *tszColname, size_t length);
extern int  haveMathMod;
extern TCHAR *mathModDelimiter;
extern unsigned int g_ctable_size;


/*
 * old code (textformat plugin dealing directly in the edit control - not the best solution, but the author
 * had no other choice as srmm never had an api for this...
 */

static TCHAR *w_bbcodes_begin[] = { _T("[b]"), _T("[i]"), _T("[u]"), _T("[s]"), _T("[color=") };
static TCHAR *w_bbcodes_end[] = { _T("[/b]"), _T("[/i]"), _T("[/u]"), _T("[/s]"), _T("[/color]") };

static TCHAR *formatting_strings_begin[] = { _T("b1 "), _T("i1 "), _T("u1 "), _T("s1 "), _T("c1 ") };
static TCHAR *formatting_strings_end[] = { _T("b0 "), _T("i0 "), _T("u0 "), _T("s0 "), _T("c0 ") };

#define NR_CODES 5
/*
 * this translates formatting tags into rtf sequences...
 * flags: loword = words only for simple  * /_ formatting
 *        hiword = bbcode support (strip bbcodes if 0)
 */

TCHAR *FilterEventMarkers(TCHAR *wszText)
{
	tstring text(wszText);
	INT_PTR beginmark = 0, endmark = 0;

	while (TRUE) {
		if ((beginmark = text.find(_T("~-+"))) != text.npos) {
			endmark = text.find(_T("+-~"), beginmark);
			if (endmark != text.npos && (endmark - beginmark) > 5) {
				text.erase(beginmark, (endmark - beginmark) + 3);
				continue;
			} else
				break;
		} else
			break;
	}
	//mad

	while (TRUE) {
		if ((beginmark = text.find( _T("\xAA"))) != text.npos) {
			endmark = beginmark+2;
			if (endmark != text.npos && (endmark - beginmark) > 1) {
				text.erase(beginmark, endmark - beginmark);
				continue;
			} else
				break;
		} else
			break;
	}
	//

	lstrcpy(wszText, text.c_str());
	return wszText;
}

const TCHAR *FormatRaw(_MessageWindowData *dat, const TCHAR *msg, int flags, BOOL isSent)
{
	bool 	clr_was_added = false, was_added;
	static 	tstring message(msg);
	INT_PTR beginmark = 0, endmark = 0, tempmark = 0, index;
	int 	i, endindex;
	TCHAR 	endmarker;
	DWORD	dwFlags = dat->dwFlags;
	message.assign(msg);



	if (haveMathMod && mathModDelimiter && message.find(mathModDelimiter) != message.npos)
		return(message.c_str());

	if(dwFlags & MWF_LOG_BBCODE) {
		if (haveMathMod && mathModDelimiter) {
			INT_PTR mark = 0;
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
				size_t closing = message.find_first_of(_T("]"), beginmark);
				was_added = false;

				if (closing == message.npos) {                      // must be an invalid [color=] tag w/o closing bracket
					message[beginmark] = ' ';
					continue;
				} else {
					tstring colorname = message.substr(beginmark + 7, 8);
search_again:
					bool  clr_found = false;
					unsigned int ii = 0;
					TCHAR szTemp[5];
					for (ii = 0; ii < g_ctable_size; ii++) {
						if (!_tcsnicmp((TCHAR *)colorname.c_str(), rtf_ctable[ii].szName, lstrlen(rtf_ctable[ii].szName))) {
							closing = beginmark + 7 + lstrlen(rtf_ctable[ii].szName);
							if (endmark != message.npos) {
								message.erase(endmark, 4);
								message.replace(endmark, 4, _T("c0 "));
							}
							message.erase(beginmark, (closing - beginmark));
							message.insert(beginmark, _T("cxxx "));
							_sntprintf(szTemp, 4, _T("%02d"), MSGDLGFONTCOUNT + 13 + ii);
							message[beginmark + 3] = szTemp[0];
							message[beginmark + 4] = szTemp[1];
							clr_found = true;
							if (was_added) {
								TCHAR wszTemp[100];
								_sntprintf(wszTemp, 100, _T("##col##%06u:%04u"), endmark - closing, ii);
								wszTemp[99] = 0;
								message.insert(beginmark, wszTemp);
							}
							break;
						}
					}
					if (!clr_found) {
						size_t  c_closing = colorname.find_first_of(_T("]"), 0);
						if (c_closing == colorname.npos)
							c_closing = colorname.length();
						const TCHAR *wszColname = colorname.c_str();
						if (endmark != message.npos && c_closing > 2 && c_closing <= 6 && iswalnum(colorname[0]) && iswalnum(colorname[c_closing -1])) {
							RTF_ColorAdd(wszColname, c_closing);
							if (!was_added) {
								clr_was_added = was_added = true;
								goto search_again;
							} else
								goto invalid_code;
						} else {
invalid_code:
							if (endmark != message.npos)
								message.erase(endmark, 8);
							if (closing != message.npos && closing < (size_t)endmark)
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
			message.insert(beginmark, _T(" "));
			message.replace(beginmark, 4, formatting_strings_begin[i]);
		}
	}

	if (!(dwFlags & MWF_LOG_TEXTFORMAT) || message.find(_T("://")) != message.npos) {
		dat->clr_added = clr_was_added ? TRUE : FALSE;
		return(message.c_str());
	}


	while ((beginmark = message.find_first_of(_T("*/_"), beginmark)) != message.npos) {
		endmarker = message[beginmark];
		if (LOWORD(flags)) {
			if (beginmark > 0 && !_istspace(message[beginmark - 1]) && !_istpunct(message[beginmark - 1])) {
				beginmark++;
				continue;
			}
			// search a corresponding endmarker which fulfills the criteria
			INT_PTR tempmark = beginmark + 1;
			while ((endmark = message.find(endmarker, tempmark)) != message.npos) {
				if (_istpunct(message[endmark + 1]) || _istspace(message[endmark + 1]) || message[endmark + 1] == 0 || _tcschr(_T("*/_"), message[endmark + 1]) != NULL)
					goto ok;
				tempmark = endmark + 1;
			}
			break;
		} else {
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

		if (PluginConfig.g_SmileyAddAvail && (endmark > (beginmark + 1))) {
			tstring smcode;
			smcode.assign(message, beginmark, (endmark - beginmark) + 1);
			SMADD_BATCHPARSE2 smbp = {0};
			SMADD_BATCHPARSERES *smbpr;

			smbp.cbSize = sizeof(smbp);
			smbp.Protocolname = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
			smbp.flag = SAFL_TCHAR | SAFL_PATH | (isSent ? SAFL_OUTGOING : 0);
			smbp.str = (TCHAR *)smcode.c_str();
			smbp.hContact = dat->hContact;
			smbpr = (SMADD_BATCHPARSERES *)CallService(MS_SMILEYADD_BATCHPARSE, 0, (LPARAM) & smbp);
			if (smbpr) {
				CallService(MS_SMILEYADD_BATCHFREE, 0, (LPARAM)smbpr);
				beginmark = endmark + 1;
				continue;
			}
		}
		message.insert(endmark, _T("%%%"));
		message.replace(endmark, 4, formatting_strings_end[index]);
		message.insert(beginmark, _T("%%%"));
		message.replace(beginmark, 4, formatting_strings_begin[index]);
	}
	dat->clr_added = clr_was_added ? TRUE : FALSE;
	return(message.c_str());
}

/*
 * format the title bar string for IM chat sessions using placeholders.
 * the caller must free() the returned string
 */

const TCHAR *NewTitle(const _MessageWindowData *dat, const TCHAR *szFormat)
{
	TCHAR *szResult = 0;
	INT_PTR length = 0;
	INT_PTR tempmark = 0;
	TCHAR szTemp[512];

	if(dat == 0)
		return(0);

	tstring title(szFormat);

	for ( size_t curpos = 0; curpos < title.length(); ) {
		if(title[curpos] != '%') {
			curpos++;
			continue;
		}
		tempmark = curpos;
		curpos++;
		if(title[curpos] == 0)
			break;

		switch (title[curpos]) {
			case 'n': {
				if (dat->szNickname)
					title.insert(tempmark + 2, dat->szNickname);
				title.erase(tempmark, 2);
				curpos = tempmark + lstrlen(dat->szNickname);
				break;
			}
			case 'p':
			case 'a': {
				if (dat->szAccount)
					title.insert(tempmark + 2, dat->szAccount);
				title.erase(tempmark, 2);
				curpos = tempmark + lstrlen(dat->szAccount);
				break;
			}
			case 's': {
				if (dat->szStatus && dat->szStatus[0])
					title.insert(tempmark + 2, dat->szStatus);
				title.erase(tempmark, 2);
				curpos = tempmark + lstrlen(dat->szStatus);
				break;
			}
			case 'u': {
				if (dat->uin[0])
					title.insert(tempmark + 2, dat->uin);
				title.erase(tempmark, 2);
				curpos = tempmark + lstrlen(dat->uin);
				break;
			}
			case 'c': {
				title.insert(tempmark + 2, dat->pContainer->szName);
				title.erase(tempmark, 2);
				curpos = tempmark + lstrlen(dat->pContainer->szName);
				break;
			}
			case 'o': {
				char	*szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
				if (szProto) {
#if defined(_UNICODE)
					MultiByteToWideChar(CP_ACP, 0, szProto, -1, szTemp, 500);
					title.insert(tempmark + 2, szTemp);
#else
					_snprintf(szTemp, 512, "%s", szProto);
					szTemp[511] = 0;
					title.insert(tempmark + 2, szTemp);
#endif
				}
				title.erase(tempmark, 2);
				curpos = tempmark + lstrlen(szTemp);
				break;
			}
			case 'x': {
				TCHAR *szFinalStatus = NULL;

				if (dat->wStatus != ID_STATUS_OFFLINE && dat->xStatus > 0 && dat->xStatus <= 31) {
					DBVARIANT dbv = {0};

					if (!M->GetTString(dat->hContact, (char *)dat->szProto, "XStatusName", &dbv)) {
						_tcsncpy(szTemp, dbv.ptszVal, 500);
						szTemp[500] = 0;
						DBFreeVariant(&dbv);
						title.insert(tempmark + 2, szTemp);
						curpos = tempmark + lstrlen(szTemp);
					}
					else {
						title.insert(tempmark + 2, xStatusDescr[dat->xStatus - 1]);
						curpos = tempmark + lstrlen(xStatusDescr[dat->xStatus - 1]);
					}
				}
				title.erase(tempmark, 2);
				break;
			}
			case 'm': {
				TCHAR *szFinalStatus = NULL;

				if (dat->wStatus != ID_STATUS_OFFLINE && dat->xStatus > 0 && dat->xStatus <= 31) {
					DBVARIANT dbv = {0};

					if (!M->GetTString(dat->hContact, (char *)dat->szProto, "XStatusName", &dbv)) {
						_tcsncpy(szTemp, dbv.ptszVal, 500);
						szTemp[500] = 0;
						DBFreeVariant(&dbv);
						title.insert(tempmark + 2, szTemp);
					} else
						szFinalStatus = xStatusDescr[dat->xStatus - 1];
				} else
					szFinalStatus = (TCHAR *)(dat->szStatus && dat->szStatus[0] ? dat->szStatus : _T("(undef)"));

				if (szFinalStatus) {
					title.insert(tempmark + 2, szFinalStatus);
					curpos = tempmark + lstrlen(szFinalStatus);
				}

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

#if defined(_UNICODE)
char *FilterEventMarkers(char *szText)
{
	std::string text(szText);
	INT_PTR beginmark = 0, endmark = 0;

	while (TRUE) {
		if ((beginmark = text.find("~-+")) != text.npos) {
			endmark = text.find("+-~", beginmark);
			if (endmark != text.npos && (endmark - beginmark) > 5) {
				text.erase(beginmark, (endmark - beginmark) + 3);
				continue;
			} else
				break;
		} else
			break;
	}
	//mad
	while (TRUE) {
		if ((beginmark = text.find( "\xAA")) != text.npos) {
			endmark = beginmark+2;
			if (endmark != text.npos && (endmark - beginmark) > 1) {
				text.erase(beginmark, endmark - beginmark);
				continue;
			} else
				break;
		} else
			break;
	}
	//
	lstrcpyA(szText, text.c_str());
	return szText;
}
#endif

// __T() not defined by MingW32

#ifdef __GNUC__
	#if defined(UNICODE)
		#define __T(x) L ## x
	#else
		#define __T(x) x
	#endif
#endif

const TCHAR *DoubleAmpersands(TCHAR *pszText)
{
	tstring text(pszText);

	INT_PTR textPos = 0;

	while (TRUE) {
		if ((textPos = text.find(_T("&"),textPos)) != text.npos) {
			text.insert(textPos,__T("%"));
			text.replace(textPos, 2, __T("&&"));
			textPos+=2;
			continue;
		} else
			break;
	}
	_tcscpy(pszText, text.c_str());
	return pszText;
}
