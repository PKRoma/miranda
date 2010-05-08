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
 * generic utility functions
 *
 */

#include "commonheaders.h"
#include <string>

#define MWF_LOG_BBCODE 1
#define MWF_LOG_TEXTFORMAT 0x2000000
#define MSGDLGFONTCOUNT 22

int				Utils::rtf_ctable_size = 0;
TRTFColorTable* Utils::rtf_ctable = 0;

static TCHAR *w_bbcodes_begin[] = { _T("[b]"), _T("[i]"), _T("[u]"), _T("[s]"), _T("[color=") };
static TCHAR *w_bbcodes_end[] = { _T("[/b]"), _T("[/i]"), _T("[/u]"), _T("[/s]"), _T("[/color]") };

static TCHAR *formatting_strings_begin[] = { _T("b1 "), _T("i1 "), _T("u1 "), _T("s1 "), _T("c1 ") };
static TCHAR *formatting_strings_end[] = { _T("b0 "), _T("i0 "), _T("u0 "), _T("s0 "), _T("c0 ") };

#define NR_CODES 5

LRESULT TSAPI _dlgReturn(HWND hWnd, LRESULT result)
{
	SetWindowLongPtr(hWnd, DWLP_MSGRESULT, result);
	return(result);
}

TCHAR* Utils::FilterEventMarkers(TCHAR *wszText)
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

/**
 * this translates formatting tags into rtf sequences...
 * flags: loword = words only for simple  * /_ formatting
 *        hiword = bbcode support (strip bbcodes if 0)
 */
const TCHAR* Utils::FormatRaw(TWindowData *dat, const TCHAR *msg, int flags, BOOL isSent)
{
	bool 	clr_was_added = false, was_added;
	static 	tstring message(msg);
	INT_PTR beginmark = 0, endmark = 0, tempmark = 0, index;
	int 	i, endindex;
	TCHAR 	endmarker;
	DWORD	dwFlags = dat->dwFlags;
	message.assign(msg);
	int		haveMathMod = PluginConfig.m_MathModAvail;
	TCHAR*	mathModDelimiter = PluginConfig.m_MathModStartDelimiter;


	if (haveMathMod && mathModDelimiter[0] && message.find(mathModDelimiter) != message.npos)
		return(message.c_str());

	if(dwFlags & MWF_LOG_BBCODE) {
		if (haveMathMod && mathModDelimiter[0]) {
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
					int ii = 0;
					TCHAR szTemp[5];
					for (ii = 0; ii < rtf_ctable_size; ii++) {
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
			smbp.Protocolname = dat->cache->getActiveProto();
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

/**
 * format the title bar string for IM chat sessions using placeholders.
 * the caller must free() the returned string
 */
const TCHAR* Utils::FormatTitleBar(const TWindowData *dat, const TCHAR *szFormat)
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
				const	TCHAR *tszNick = dat->cache->getNick();
				if (tszNick[0])
					title.insert(tempmark + 2, tszNick);
				title.erase(tempmark, 2);
				curpos = tempmark + lstrlen(tszNick);
				break;
			}
			case 'p':
			case 'a': {
				const	TCHAR *szAcc = dat->cache->getRealAccount();
				if (szAcc)
					title.insert(tempmark + 2, szAcc);
				title.erase(tempmark, 2);
				curpos = tempmark + lstrlen(szAcc);
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
				const TCHAR	*szUIN = dat->cache->getUIN();
				if (szUIN[0])
					title.insert(tempmark + 2, szUIN);
				title.erase(tempmark, 2);
				curpos = tempmark + lstrlen(szUIN);
				break;
			}
			case 'c': {
				TCHAR	*c = (!_tcscmp(dat->pContainer->szName, _T("default")) ? TranslateTS(dat->pContainer->szName) : dat->pContainer->szName);
				title.insert(tempmark + 2, c);
				title.erase(tempmark, 2);
				curpos = tempmark + lstrlen(c);
				break;
			}
			case 'o': {
				const TCHAR* szProto = dat->cache->getActiveProtoT();
				if (szProto)
					title.insert(tempmark + 2, szProto);
				title.erase(tempmark, 2);
				curpos = tempmark + (szProto ? lstrlen(szProto) : 0);
				break;
			}
			case 'x': {
				TCHAR *szFinalStatus = NULL;
				BYTE  xStatus = dat->cache->getXStatusId();

				if (dat->wStatus != ID_STATUS_OFFLINE && xStatus > 0 && xStatus <= 31) {
					DBVARIANT dbv = {0};

					if (!M->GetTString(dat->hContact, (char *)dat->szProto, "XStatusName", &dbv)) {
						_tcsncpy(szTemp, dbv.ptszVal, 500);
						szTemp[500] = 0;
						DBFreeVariant(&dbv);
						title.insert(tempmark + 2, szTemp);
						curpos = tempmark + lstrlen(szTemp);
					}
					else {
						title.insert(tempmark + 2, xStatusDescr[xStatus - 1]);
						curpos = tempmark + lstrlen(xStatusDescr[xStatus - 1]);
					}
				}
				title.erase(tempmark, 2);
				break;
			}
			case 'm': {
				TCHAR *szFinalStatus = NULL;
				BYTE  xStatus = dat->cache->getXStatusId();

				if (dat->wStatus != ID_STATUS_OFFLINE && xStatus > 0 && xStatus <= 31) {
					DBVARIANT dbv = {0};

					if (!M->GetTString(dat->hContact, (char *)dat->szProto, "XStatusName", &dbv)) {
						_tcsncpy(szTemp, dbv.ptszVal, 500);
						szTemp[500] = 0;
						DBFreeVariant(&dbv);
						title.insert(tempmark + 2, szTemp);
					} else
						szFinalStatus = xStatusDescr[xStatus - 1];
				} else
					szFinalStatus = (TCHAR *)(dat->szStatus && dat->szStatus[0] ? dat->szStatus : _T("(undef)"));

				if (szFinalStatus) {
					title.insert(tempmark + 2, szFinalStatus);
					curpos = tempmark + lstrlen(szFinalStatus);
				}

				title.erase(tempmark, 2);
				break;
			}
			/*
			 * status message (%T will skip the "No status message" for empty
			 * messages)
			 */
			case 't':
			case 'T': {
				TCHAR	*tszStatusMsg = dat->cache->getNormalizedStatusMsg(dat->cache->getStatusMsg(), true);

				if(tszStatusMsg) {
					title.insert(tempmark + 2, tszStatusMsg);
					curpos = tempmark + lstrlen(tszStatusMsg);
				}
				else if(title[curpos] == 't') {
					const TCHAR* tszStatusMsg = CTranslator::get(CTranslator::GEN_NO_STATUS);
					title.insert(tempmark + 2, tszStatusMsg);
					curpos = tempmark + lstrlen(tszStatusMsg);
				}
				title.erase(tempmark, 2);
				if(tszStatusMsg)
					mir_free(tszStatusMsg);
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
char* Utils::FilterEventMarkers(char *szText)
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

const TCHAR* Utils::DoubleAmpersands(TCHAR *pszText)
{
	tstring text(pszText);

	INT_PTR textPos = 0;

	while (TRUE) {
		if ((textPos = text.find(_T("&"),textPos)) != text.npos) {
			text.insert(textPos,_T("%"));
			text.replace(textPos, 2, _T("&&"));
			textPos+=2;
			continue;
		} else
			break;
	}
	_tcscpy(pszText, text.c_str());
	return pszText;
}

/**
 * Get a preview of the text with an ellipsis appended (...)
 *
 * @param szText	source text
 * @param iMaxLen	max length of the preview
 * @return TCHAR*   result (caller must mir_free() it)
 */
TCHAR* Utils::GetPreviewWithEllipsis(TCHAR *szText, size_t iMaxLen)
{
	size_t   uRequired;
	TCHAR*	 p = 0, cSaved;
	bool	 fEllipsis = false;

	if(_tcslen(szText) <= iMaxLen)
		uRequired = _tcslen(szText) + 4;
	else {
		TCHAR *p = &szText[iMaxLen - 1];
		fEllipsis = true;

		while(p >= szText && *p != ' ')
			p--;
		if(p == szText)
			p = szText + iMaxLen - 1;

		cSaved = *p;
		*p = 0;
		uRequired = (p - szText) + 6;
	}
	TCHAR *szResult = reinterpret_cast<TCHAR *>(mir_alloc(uRequired * sizeof(TCHAR)));
	mir_sntprintf(szResult, uRequired, fEllipsis ? _T("%s...") : _T("%s"), szText);

	if(p)
		*p = cSaved;

	return(szResult);
}

/*
 * returns != 0 when one of the installed keyboard layouts belongs to an rtl language
 * used to find out whether we need to configure the message input box for bidirectional mode
 */

int Utils::FindRTLLocale(TWindowData *dat)
{
	HKL layouts[20];
	int i, result = 0;
	LCID lcid;
	WORD wCtype2[5];

	if (dat->iHaveRTLLang == 0) {
		ZeroMemory(layouts, 20 * sizeof(HKL));
		GetKeyboardLayoutList(20, layouts);
		for (i = 0; i < 20 && layouts[i]; i++) {
			lcid = MAKELCID(LOWORD(layouts[i]), 0);
			GetStringTypeA(lcid, CT_CTYPE2, "���", 3, wCtype2);
			if (wCtype2[0] == C2_RIGHTTOLEFT || wCtype2[1] == C2_RIGHTTOLEFT || wCtype2[2] == C2_RIGHTTOLEFT)
				result = 1;
		}
		dat->iHaveRTLLang = (result ? 1 : -1);
	} else
		result = dat->iHaveRTLLang == 1 ? 1 : 0;

	return result;
}

/*
 * init default color table. the table may grow when using custom colors via bbcodes
 */

void Utils::RTF_CTableInit()
{
	int iSize = sizeof(TRTFColorTable) * RTF_CTABLE_DEFSIZE;

	rtf_ctable = (TRTFColorTable *)malloc(iSize);
	ZeroMemory(rtf_ctable, iSize);
	CopyMemory(rtf_ctable, _rtf_ctable, iSize);
	rtf_ctable_size = RTF_CTABLE_DEFSIZE;
}

/*
 * add a color to the global rtf color table
 */

void Utils::RTF_ColorAdd(const TCHAR *tszColname, size_t length)
{
	TCHAR *stopped;
	COLORREF clr;

	rtf_ctable_size++;
	rtf_ctable = (TRTFColorTable *)realloc(rtf_ctable, sizeof(TRTFColorTable) * rtf_ctable_size);
	clr = _tcstol(tszColname, &stopped, 16);
	mir_sntprintf(rtf_ctable[rtf_ctable_size - 1].szName, length + 1, _T("%06x"), clr);
	rtf_ctable[rtf_ctable_size - 1].menuid = rtf_ctable[rtf_ctable_size - 1].index = 0;

	clr = _tcstol(tszColname, &stopped, 16);
	rtf_ctable[rtf_ctable_size - 1].clr = (RGB(GetBValue(clr), GetGValue(clr), GetRValue(clr)));
}

void Utils::CreateColorMap(TCHAR *Text)
{
	TCHAR * pszText = Text;
	TCHAR * p1;
	TCHAR * p2;
	TCHAR * pEnd;
	int iIndex = 1, i = 0;
	COLORREF default_color;

	static const TCHAR *lpszFmt = _T("\\red%[^ \x5b\\]\\green%[^ \x5b\\]\\blue%[^ \x5b;];");
	TCHAR szRed[10], szGreen[10], szBlue[10];

	p1 = _tcsstr(pszText, _T("\\colortbl"));
	if (!p1)
		return;

	pEnd = _tcschr(p1, '}');

	p2 = _tcsstr(p1, _T("\\red"));

	for (i = 0; i < RTF_CTABLE_DEFSIZE; i++)
		rtf_ctable[i].index = 0;

	default_color = (COLORREF)M->GetDword(FONTMODULE, "Font16Col", 0);

	while (p2 && p2 < pEnd) {
		if (_stscanf(p2, lpszFmt, &szRed, &szGreen, &szBlue) > 0) {
			int i;
			for (i = 0; i < RTF_CTABLE_DEFSIZE; i++) {
				if (rtf_ctable[i].clr == RGB(_ttoi(szRed), _ttoi(szGreen), _ttoi(szBlue)))
					rtf_ctable[i].index = iIndex;
			}
		}
		iIndex++;
		p1 = p2;
		p1 ++;

		p2 = _tcsstr(p1, _T("\\red"));
	}
	return ;
}

int Utils::RTFColorToIndex(int iCol)
{
	int i = 0;
	for (i = 0; i < RTF_CTABLE_DEFSIZE; i++) {
		if (rtf_ctable[i].index == iCol)
			return i + 1;
	}
	return 0;
}

/**
 * generic error popup dialog procedure
 */
INT_PTR CALLBACK Utils::PopupDlgProcError(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd, (LPARAM)&hContact);

	switch (message) {
	case WM_COMMAND:
		PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_HANDLECLISTEVENT, (WPARAM)hContact, 0);
		PUDeletePopUp(hWnd);
		break;
	case WM_CONTEXTMENU:
		PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_HANDLECLISTEVENT, (WPARAM)hContact, 0);
		PUDeletePopUp(hWnd);
		break;
	case WM_MOUSEWHEEL:
		break;
	case WM_SETCURSOR:
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/**
 * read a blob from db into the container settings structure
 * @param hContact:	contact handle (0 = read global)
 * @param cs		TContainerSettings* target structure
 * @return			0 on success, 1 failure (blob does not exist OR is not a valid private setting structure
 */
int Utils::ReadContainerSettingsFromDB(const HANDLE hContact, TContainerSettings *cs, const char *szKey)
{
	DBVARIANT 	dbv = {0};

	CopyMemory(cs, &PluginConfig.globalContainerSettings, sizeof(TContainerSettings));

	if(0 == DBGetContactSetting(hContact, SRMSGMOD_T, szKey ? szKey : CNT_KEYNAME, &dbv)) {
		if(dbv.type == DBVT_BLOB && dbv.cpbVal > 0 && dbv.cpbVal <= sizeof(TContainerSettings)) {
			::CopyMemory((void *)cs, (void *)dbv.pbVal, dbv.cpbVal);
			::DBFreeVariant(&dbv);
			if(hContact == 0 && szKey == 0)
				cs->fPrivate = false;
			return(0);
		}
		cs->fPrivate = false;
		DBFreeVariant(&dbv);
		return(1);
	}
	else {
		cs->fPrivate = false;
		return(1);
	}
}

int Utils::WriteContainerSettingsToDB(const HANDLE hContact, TContainerSettings *cs, const char *szKey)
{
	DBWriteContactSettingBlob(hContact, SRMSGMOD_T, szKey ? szKey : CNT_KEYNAME, cs, sizeof(TContainerSettings));
	return(0);
}

void Utils::SettingsToContainer(TContainerData *pContainer)
{
	pContainer->dwFlags 		= pContainer->settings->dwFlags;
	pContainer->dwFlagsEx 		= pContainer->settings->dwFlagsEx;
	pContainer->avatarMode 		= pContainer->settings->avatarMode;
	pContainer->ownAvatarMode 	= pContainer->settings->ownAvatarMode;
}

void Utils::ContainerToSettings(TContainerData *pContainer)
{
	pContainer->settings->dwFlags			= pContainer->dwFlags;
	pContainer->settings->dwFlagsEx			= pContainer->dwFlagsEx;
	pContainer->settings->avatarMode		= pContainer->avatarMode;
	pContainer->settings->ownAvatarMode		= pContainer->ownAvatarMode;
}

/**
 * read settings for a container with private settings enabled.
 *
 * @param pContainer	container window info struct
 * @param fForce		true -> force them private, even if they were not marked as private in the db
 */
void Utils::ReadPrivateContainerSettings(TContainerData *pContainer, bool fForce)
{
	char	szCname[50];
	TContainerSettings csTemp = {0};

	mir_snprintf(szCname, 40, "%s%d_Blob", CNT_BASEKEYNAME, pContainer->iContainerIndex);
	Utils::ReadContainerSettingsFromDB(0, &csTemp, szCname);
	if(csTemp.fPrivate || fForce) {
		if(pContainer->settings == 0 || pContainer->settings == &PluginConfig.globalContainerSettings)
			pContainer->settings = (TContainerSettings *)malloc(sizeof(TContainerSettings));
		CopyMemory((void *)pContainer->settings, (void *)&csTemp, sizeof(TContainerSettings));
		pContainer->settings->fPrivate = true;
	}
	else
		pContainer->settings = &PluginConfig.globalContainerSettings;
}

void Utils::SaveContainerSettings(TContainerData *pContainer, const char *szSetting)
{
	char	szCName[50];

	pContainer->dwFlags &= ~(CNT_DEFERREDCONFIGURE | CNT_CREATE_MINIMIZED | CNT_DEFERREDSIZEREQUEST | CNT_CREATE_CLONED);
	if(pContainer->settings->fPrivate) {
		_snprintf(szCName, 40, "%s%d_Blob", szSetting, pContainer->iContainerIndex);
		WriteContainerSettingsToDB(0, pContainer->settings, szCName);
	}
	mir_snprintf(szCName, 40, "%s%d_theme", szSetting, pContainer->iContainerIndex);
	if (lstrlen(pContainer->szRelThemeFile) > 1) {
		if(pContainer->fPrivateThemeChanged == TRUE) {
			M->pathToRelative(pContainer->szRelThemeFile, pContainer->szAbsThemeFile);
			M->WriteTString(NULL, SRMSGMOD_T, szCName, pContainer->szAbsThemeFile);
			pContainer->fPrivateThemeChanged = FALSE;
		}
	}
	else {
		::DBDeleteContactSetting(NULL, SRMSGMOD_T, szCName);
		pContainer->fPrivateThemeChanged = FALSE;
	}
}

/**
 * calculate new width and height values for a user picture (avatar)
 * 
 * @param: maxHeight -	determines maximum height for the picture, width will
 *						be scaled accordingly.
 */
void Utils::scaleAvatarHeightLimited(const HBITMAP hBm, double& dNewWidth, double& dNewHeight, LONG maxHeight)
{
	BITMAP	bm;
	double	dAspect;

	GetObject(hBm, sizeof(bm), &bm);

	if (bm.bmHeight > bm.bmWidth) {
		if (bm.bmHeight > 0)
			dAspect = (double)(maxHeight) / (double)bm.bmHeight;
		else
			dAspect = 1.0;
		dNewWidth = (double)bm.bmWidth * dAspect;
		dNewHeight = (double)maxHeight;
	} else {
		if (bm.bmWidth > 0)
			dAspect = (double)(maxHeight) / (double)bm.bmWidth;
		else
			dAspect = 1.0;
		dNewHeight = (double)bm.bmHeight * dAspect;
		dNewWidth = (double)maxHeight;
	}
}

/**
 * convert the avatar bitmap to icon format so that it can be used on the task bar
 * tries to keep correct aspect ratio of the avatar image
 *
 * @param dat: _MessageWindowData* pointer to the window data
 * @return HICON: the icon handle
 */
HICON Utils::iconFromAvatar(const TWindowData *dat)
{
	double		dNewWidth, dNewHeight;
	bool		fFree = false;
	HIMAGELIST 	hIml_c = 0;
	HICON		hIcon = 0;

	if(!ServiceExists(MS_AV_GETAVATARBITMAP))
		return(0);

	if(dat) {
		AVATARCACHEENTRY *ace = (AVATARCACHEENTRY *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)dat->hContact, 0);
		LONG	lIconSize = Win7Taskbar->getIconSize();

		if(ace && ace->hbmPic) {
			scaleAvatarHeightLimited(ace->hbmPic, dNewWidth, dNewHeight, lIconSize);
			/*
			 * resize picture to fit it on the task bar, use an image list for converting it to
			 * 32bpp icon format
			 * dat->hTaskbarIcon will cache it until avatar is changed
			 */
			HBITMAP hbmResized = CSkin::ResizeBitmap(ace->hbmPic, (LONG)dNewWidth, (LONG)dNewHeight, fFree);
			hIml_c = ::ImageList_Create(lIconSize, lIconSize, ILC_COLOR32 | ILC_MASK, 1, 0);

			RECT	rc = {0, 0, lIconSize, lIconSize};

			HDC 	hdc = ::GetDC(dat->pContainer->hwnd);
			HDC 	dc = ::CreateCompatibleDC(hdc);
			HDC 	dcResized = ::CreateCompatibleDC(hdc);

			ReleaseDC(dat->pContainer->hwnd, hdc);

			HBITMAP hbmNew = CSkin::CreateAeroCompatibleBitmap(rc, dc);
			HBITMAP hbmOld = reinterpret_cast<HBITMAP>(::SelectObject(dc, hbmNew));
			HBITMAP hbmOldResized = reinterpret_cast<HBITMAP>(::SelectObject(dcResized, hbmResized));

			LONG	ix = (lIconSize - (LONG)dNewWidth) / 2;
			LONG	iy = (lIconSize - (LONG)dNewHeight) / 2;
			CSkin::m_default_bf.SourceConstantAlpha = M->GetByte("taskBarIconAlpha", 255);
			CMimAPI::m_MyAlphaBlend(dc, ix, iy, (LONG)dNewWidth, (LONG)dNewHeight, dcResized,
									0, 0, (LONG)dNewWidth, (LONG)dNewHeight, CSkin::m_default_bf);

			CSkin::m_default_bf.SourceConstantAlpha = 255;
			::SelectObject(dc, hbmOld);
			::ImageList_Add(hIml_c, hbmNew, 0);
			::DeleteObject(hbmNew);
			::DeleteDC(dc);

			::SelectObject(dcResized, hbmOldResized);
			::DeleteObject(hbmResized);
			::DeleteDC(dcResized);
			hIcon = ::ImageList_GetIcon(hIml_c, 0, ILD_NORMAL);
			::ImageList_RemoveAll(hIml_c);
			::ImageList_Destroy(hIml_c);
		}
	}
	return(hIcon);
}

AVATARCACHEENTRY* Utils::loadAvatarFromAVS(const HANDLE hContact)
{
	if(ServiceExists(MS_AV_GETAVATARBITMAP))
		return(reinterpret_cast<AVATARCACHEENTRY *>(CallService(MS_AV_GETAVATARBITMAP, (WPARAM)hContact, 0)));
	else
		return(0);
}

void Utils::getIconSize(HICON hIcon, int& sizeX, int& sizeY)
{
	ICONINFO ii;
	BITMAP bm;
	::GetIconInfo(hIcon, &ii);
	::GetObject(ii.hbmColor, sizeof(bm), &bm);
	sizeX = bm.bmWidth;
	sizeY = bm.bmHeight;
	::DeleteObject(ii.hbmMask);
	::DeleteObject(ii.hbmColor);
}

/**
 * add a menu item to a ownerdrawn menu. mii must be pre-initialized
 *
 * @param m			menu handle
 * @param mii		menu item info structure
 * @param hIcon		the icon (0 allowed -> no icon)
 * @param szText	menu item text (must NOT be 0)
 * @param uID		the item command id
 * @param pos		zero-based position index
 */
void Utils::addMenuItem(const HMENU& m, MENUITEMINFO& mii, HICON hIcon, const TCHAR *szText, UINT uID, UINT pos)
{
	mii.wID = uID;
	mii.dwItemData = (ULONG_PTR)hIcon;
	mii.dwTypeData = const_cast<TCHAR *>(szText);
	mii.cch = lstrlen(mii.dwTypeData) + 1;

	::InsertMenuItem(m, pos, TRUE, &mii);
}

/**
 * return != 0 when the sound effect must be played for the given
 * session. Uses container sound settings
 */
int	TSAPI Utils::mustPlaySound(const TWindowData *dat)
{
	if(!dat)
		return(0);

	if(dat->pContainer->fHidden)		// hidden container is treated as closed, so play the sound
		return(1);

	if(dat->pContainer->dwFlags & CNT_NOSOUND || nen_options.iNoSounds)
		return(0);

	bool fActiveWindow = (dat->pContainer->hwnd == ::GetForegroundWindow() ? true : false);
	bool fActiveTab = (dat->pContainer->hwndActive == dat->hwnd ? true : false);
	bool fIconic = (::IsIconic(dat->pContainer->hwnd) ? true : false);

	/*
	 * window minimized, check if sound has to be played
	 */
	if(fIconic)
		return(dat->pContainer->dwFlagsEx & CNT_EX_SOUNDS_MINIMIZED ? 1 : 0);

	/*
	 * window in foreground
	 */
	if(fActiveWindow) {
		if(fActiveTab)
			return(dat->pContainer->dwFlagsEx & CNT_EX_SOUNDS_FOCUSED ? 1 : 0);
		else
			return(dat->pContainer->dwFlagsEx & CNT_EX_SOUNDS_INACTIVETABS ? 1 : 0);
	}
	else
		return(dat->pContainer->dwFlagsEx & CNT_EX_SOUNDS_UNFOCUSED ? 1 : 0);

	return(1);
}

/**
 * enable or disable a dialog control
 */
void TSAPI Utils::enableDlgControl(const HWND hwnd, UINT id, BOOL fEnable)
{
	::EnableWindow(::GetDlgItem(hwnd, id), fEnable);
}

/**
 * show or hide a dialog control
 */
void TSAPI Utils::showDlgControl(const HWND hwnd, UINT id, int showCmd)
{
	::ShowWindow(::GetDlgItem(hwnd, id), showCmd);
}

/*
 * stream function to write the contents of the message log to an rtf file
 */

DWORD CALLBACK Utils::StreamOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
	HANDLE hFile;
	TCHAR *szFilename = (TCHAR *)dwCookie;
	if ((hFile = CreateFile(szFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
		SetFilePointer(hFile, 0, NULL, FILE_END);
		WriteFile(hFile, pbBuff, cb, (DWORD *)pcb, NULL);
		*pcb = cb;
		CloseHandle(hFile);
		return 0;
	}
	return 1;
}

/**
 * extract a resource from the given module
 * tszPath must end with \
 */
void TSAPI Utils::extractResource(const HMODULE h, const UINT uID, const TCHAR *tszName, const TCHAR *tszPath,
								  const TCHAR *tszFilename, bool fForceOverwrite)
{
	HRSRC 	hRes;
	HGLOBAL	hResource;
	TCHAR	szFilename[MAX_PATH];

	hRes = FindResource(h, MAKEINTRESOURCE(uID), tszName);

	if(hRes) {
		hResource = LoadResource(h, hRes);
		if(hResource) {
			HANDLE  hFile;
			char 	*pData = (char *)LockResource(hResource);
			DWORD	dwSize = SizeofResource(g_hInst, hRes), written = 0;
			mir_sntprintf(szFilename, MAX_PATH, _T("%s%s"), tszPath, tszFilename);
			if(!fForceOverwrite) {
				if(PathFileExists(szFilename))
					return;
			}
			if((hFile = CreateFile(szFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) != INVALID_HANDLE_VALUE) {
				WriteFile(hFile, (void *)pData, dwSize, &written, NULL);
				CloseHandle(hFile);
			}
			else
				throw(CRTException("Error while extracting aero skin images, Aero mode disabled.", szFilename));
		}
	}
}

/**
 * generic command dispatcher
 * used in various places (context menus, info panel menus etc.)
 */
LRESULT Utils::CmdDispatcher(UINT uType, HWND hwndDlg, UINT cmd, WPARAM wParam, LPARAM lParam, TWindowData *dat, TContainerData *pContainer)
{
	switch(uType) {
		case CMD_CONTAINER:
			if(pContainer && hwndDlg)
				return(DM_ContainerCmdHandler(pContainer, cmd, wParam, lParam));
			break;
		case CMD_MSGDIALOG:
			if(pContainer && hwndDlg && dat)
				return(DM_MsgWindowCmdHandler(hwndDlg, pContainer, dat, cmd, wParam, lParam));
			break;
		case CMD_INFOPANEL:
			if(MsgWindowMenuHandler(dat, cmd, MENU_LOGMENU) == 0) {
				return(DM_MsgWindowCmdHandler(hwndDlg, pContainer, dat, cmd, wParam, lParam));
			}
			break;
	}
	return(0);
}
