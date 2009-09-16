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
 * highlighter class for multi user chats
 *
 */

#include "../src/commonheaders.h"

//#define __HLT_PERFSTATS 1

void CMUCHighlight::cleanup()
{
	if(m_NickPatternString)
		mir_free(m_NickPatternString);
	if(m_TextPatternString)
		mir_free(m_TextPatternString);

	m_NickPatterns.clear();
	m_TextPatterns.clear();
}

void CMUCHighlight::init()
{
	DBVARIANT dbv = {0};

	if(m_fInitialized)
		cleanup();							// clean up first, if we were already initialized

	m_fInitialized = true;

	if(0 == M->GetTString(0, "Chat", "HighlightWords", &dbv)) {
		m_TextPatternString = dbv.ptszVal;
		_tcslwr(m_TextPatternString);
	}

	if(0 == M->GetTString(0, "Chat", "HighlightNames", &dbv))
		m_NickPatternString = dbv.ptszVal;

	m_dwFlags = M->GetDword("Chat", "HighlightEnabled", MATCH_TEXT);

	tokenize(m_TextPatternString, m_TextPatterns);
	tokenize(m_NickPatternString, m_NickPatterns);

	/*
	PatternIterator it = m_TextPatterns.begin();

	while(it != m_TextPatterns.end()) {
		_DebugTraceW(_T("Text pattern: '%s'"), *it);
		it++;
	}

	it = m_NickPatterns.begin();

	while(it != m_NickPatterns.end()) {
		_DebugTraceW(_T("Nick pattern: '%s'"), *it);
		it++;
	}
	*/
}

void CMUCHighlight::tokenize(TCHAR *tszString, PatternList& v)
{
	if(tszString == 0)
		return;

	TCHAR	*p = tszString;

	if(*p == 0)
		return;

	if(*p != ' ')
		v.push_back(tszString);

	while(*p) {
		if(*p == ' ') {
			*p++ = 0;
			while(*p && _istspace(*p))
				p++;
			if(*p)
				v.push_back(p);
		}
		p++;
	}
}

int CMUCHighlight::match(const GCEVENT *pgce, const SESSION_INFO *psi, DWORD dwFlags, const TCHAR *tszAdditonal)
{
	int 			result = 0, nResult = 0;
	PatternIterator it;

	if(pgce == 0)
		return(0);

#ifdef __HLT_PERFSTATS
	__int64 lFreq, lStart, lStop;
	::QueryPerformanceFrequency((LARGE_INTEGER *)&lFreq);
	double frequency = 1.0 / (double)lFreq;
#endif

	int		words = 0;
	size_t	patterns = m_TextPatterns.size();

	if((m_dwFlags & MATCH_TEXT) && (dwFlags & MATCH_TEXT) && m_TextPatterns.size() > 0) {
#ifdef __HLT_PERFSTATS
		::QueryPerformanceCounter((LARGE_INTEGER *)&lStart);
#endif
		TCHAR	*tszCleaned = ::RemoveFormatting(pgce->ptszText, true);
		register TCHAR	*p = tszCleaned;
		register TCHAR  *p1;

		TCHAR	*tszMe = mir_tstrdup(psi->pMe->pszNick);
		_tcslwr(tszMe);
		words = 0;

		while(p) {
			while(*p && (*p == ' ' || *p == ',' || *p == '.' || *p == ':' || *p == ';' || *p == '?' || *p == '!'))
				p++;

			if(*p) {
				p1 = p;
				while(*p1 && *p1 != ' ' && *p1 != ',' && *p1 != '.' && *p1 != ':' && *p1 != ';' && *p1 != '?' && *p1 != '!')
					p1++;

				if(*p1)
					*p1 = 0;
				else
					p1 = 0;

				it = m_TextPatterns.begin();
				while(it != m_TextPatterns.end() && !result) {
					if(*(*it) == '%' && *(*it + 1) == 'm') {
						result = wildmatch(tszMe, p) ? MATCH_TEXT : 0;
					} else
						result = wildmatch(*it, p) ? MATCH_TEXT : 0;
					it++;
				}
				if(p1) {
					*p1 = ' ';
					p = p1 + 1;
				}
				else
					p = 0;
				words++;
			}
			else
				break;

			if(result)
				break;
		}

#ifdef __HLT_PERFSTATS
		::QueryPerformanceCounter((LARGE_INTEGER *)&lStop);
		if(psi && psi->dat) {
			mir_sntprintf(psi->dat->szStatusBar, 100, _T("PERF text match: %d ticks = %f msec (%d words, %d patterns)"), (int)(lStop - lStart), 1000 * ((double)(lStop - lStart) * frequency), words, patterns);
			if(psi->dat->pContainer->hwndStatus)
				::SendMessage(psi->dat->pContainer->hwndStatus, SB_SETTEXT, 0, (LPARAM)psi->dat->szStatusBar);
		}
#endif
		mir_free(tszMe);

	}
	if((m_dwFlags & MATCH_NICKNAME) && (dwFlags & MATCH_NICKNAME) && pgce->ptszNick && m_NickPatterns.size() > 0) {
		it = m_NickPatterns.begin();

		while(it != m_NickPatterns.end() && !nResult) {
			nResult = wildmatch(*it, pgce->ptszNick) ? MATCH_NICKNAME : 0;
			if(tszAdditonal) {
				nResult = wildmatch(*it, tszAdditonal) ? MATCH_NICKNAME : 0;
				//_DebugTraceW(_T("match %s against additional: %s (%d)"), *it, tszAdditonal, nResult);
			}
			if((m_dwFlags & MATCH_UIN) && pgce->ptszUserInfo) {
				nResult = wildmatch(*it, pgce->ptszUserInfo) ? MATCH_NICKNAME : 0;
				//_DebugTraceW(_T("match %s against UID: %s (%d)"), *it, pgce->ptszUserInfo, nResult);
			}
			it++;
		}
	}

	return(result | nResult);
}

int CMUCHighlight::wildmatch(const TCHAR *pattern, const TCHAR *tszString) {

	const TCHAR *cp = 0, *mp = 0;

	while ((*tszString) && (*pattern != '*')) {
		if ((*pattern != *tszString) && (*pattern != '?')) {
			return 0;
		}
		pattern++;
		tszString++;
	}

	while (*tszString) {
		if (*pattern == '*') {
			if (!*++pattern)
				return 1;
		  mp = pattern;
		  cp = tszString + 1;
		}
		else if ((*pattern == *tszString) || (*pattern == '?')) {
			pattern++;
			tszString++;
		}
		else {
			pattern = mp;
			tszString = cp++;
		}
	}

	while (*pattern == '*')
		pattern++;

	return(!*pattern);
}

/*
BOOL CMUCHighlight::textMatch(SESSION_INFO* si, const TCHAR* pszText)
{
	TCHAR* p1 = m_TextPatternString;
	TCHAR* p2 = NULL;
	const TCHAR* p3 = pszText;
	static TCHAR szWord1[1000];
	static TCHAR szWord2[1000];
	static TCHAR szTrimString[] = _T(":,.!?;\'>)");

	// compare word for word
	while (*p1 != '\0') {
		// find the next/first word in the highlight word string
		// skip 'spaces' be4 the word
		while (*p1 == ' ' && *p1 != '\0')
			p1 += 1;

		//find the end of the word
		p2 = _tcschr(p1, ' ');
		if (!p2)
			p2 = _tcschr(p1, '\0');
		if (p1 == p2)
			return FALSE;

		// copy the word into szWord1
		lstrcpyn(szWord1, p1, p2 - p1 > 998 ? 999 : p2 - p1 + 1);
		p1 = p2;

		// replace %m with the users nickname
		p2 = _tcschr(szWord1, '%');
		if (p2 && p2[1] == 'm') {
			TCHAR szTemp[50];

			p2[1] = 's';
			lstrcpyn(szTemp, szWord1, SIZEOF(szTemp));
			mir_sntprintf(szWord1, SIZEOF(szWord1), szTemp, si->pMe->pszNick);
		}

		// time to get the next/first word in the incoming text string
		while (*p3 != '\0') {
			// skip 'spaces' be4 the word
			while (*p3 == ' ' && *p3 != '\0')
				p3 += 1;

			//find the end of the word
			p2 = (TCHAR *)_tcschr(p3, ' ');
			if (!p2)
				p2 = (TCHAR *)_tcschr(p3, '\0');


			if (p3 != p2) {
				// eliminate ending character if needed
				if (p2 - p3 > 1 && _tcschr(szTrimString, p2[-1]))
					p2 -= 1;

				// copy the word into szWord2 and remove formatting
				lstrcpyn(szWord2, p3, p2 - p3 > 998 ? 999 : p2 - p3 + 1);

				// reset the pointer if it was touched because of an ending character
				if (*p2 != '\0' && *p2 != ' ')
					p2 += 1;
				p3 = p2;

				CharLower(szWord1);
				CharLower(szWord2);

				// compare the words, using wildcards
				if (WCCmp(szWord1, RemoveFormatting(szWord2)))
					return TRUE;
			}
		}
		p3 = pszText;
	}
	return FALSE;
}
*/
/**
 * Dialog procedure to handle global highlight settings
 *
 * @param Standard Windows dialog procedure parameters
 */

INT_PTR CALLBACK CMUCHighlight::dlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG: {
			DBVARIANT	dbv = {0};

			TranslateDialogDefault(hwndDlg);

			if(0 == M->GetTString(0, "Chat", "HighlightWords", &dbv)) {
				::SetDlgItemText(hwndDlg, IDC_HIGHLIGHTTEXTPATTERN, dbv.ptszVal);
				::DBFreeVariant(&dbv);
			}

			if(0 == M->GetTString(0, "Chat", "HighlightNames", &dbv)) {
				::SetDlgItemText(hwndDlg, IDC_HIGHLIGHTNICKPATTERN, dbv.ptszVal);
				::DBFreeVariant(&dbv);
			}

			DWORD dwFlags = M->GetDword("Chat", "HighlightEnabled", MATCH_TEXT);

			::CheckDlgButton(hwndDlg, IDC_HIGHLIGHTNICKENABLE, dwFlags & MATCH_NICKNAME ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwndDlg, IDC_HIGHLIGHTNICKUID, dwFlags & MATCH_UIN ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwndDlg, IDC_HIGHLIGHTTEXTENABLE, dwFlags & MATCH_TEXT ? BST_CHECKED : BST_UNCHECKED);

			::SendMessage(hwndDlg, WM_USER + 100, 0, 0);
			return(TRUE);
		}

		case WM_USER + 100:
			::EnableWindow(::GetDlgItem(hwndDlg, IDC_HIGHLIGHTTEXTPATTERN),
										::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTTEXTENABLE) ? TRUE : FALSE);

			::EnableWindow(::GetDlgItem(hwndDlg, IDC_HIGHLIGHTNICKPATTERN),
										::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTNICKENABLE) ? TRUE : FALSE);

			::EnableWindow(::GetDlgItem(hwndDlg, IDC_HIGHLIGHTNICKUID),
										::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTNICKENABLE) ? TRUE : FALSE);

			return(FALSE);

		case WM_COMMAND:
			if ((LOWORD(wParam)		  == IDC_HIGHLIGHTNICKPATTERN
					|| LOWORD(wParam) == IDC_HIGHLIGHTTEXTPATTERN)
					&& (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != ::GetFocus()))
				return 0;

			::SendMessage(hwndDlg, WM_USER + 100, 0, 0);
			if (lParam != (LPARAM)NULL)
				::SendMessage(::GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code) {
						case PSN_APPLY: {
							TCHAR	*szBuf = 0;
							int		iLen = ::GetWindowTextLength(::GetDlgItem(hwndDlg, IDC_HIGHLIGHTNICKPATTERN));

							if(iLen) {
								szBuf = reinterpret_cast<TCHAR *>(mir_alloc((iLen + 2) * sizeof(TCHAR)));
								::GetDlgItemText(hwndDlg, IDC_HIGHLIGHTNICKPATTERN, szBuf, iLen + 1);
								M->WriteTString(0, "Chat", "HighlightNames",szBuf);
							}

							iLen = ::GetWindowTextLength(::GetDlgItem(hwndDlg, IDC_HIGHLIGHTTEXTPATTERN));
							if(iLen) {
								szBuf = reinterpret_cast<TCHAR *>(mir_realloc(szBuf, sizeof(TCHAR) * (iLen + 2)));
								::GetDlgItemText(hwndDlg, IDC_HIGHLIGHTTEXTPATTERN, szBuf, iLen + 1);
								M->WriteTString(0, "Chat", "HighlightWords", szBuf);
							}
							mir_free(szBuf);
							DWORD dwFlags = (::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTNICKENABLE) ? MATCH_NICKNAME : 0) |
								(::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTTEXTENABLE) ? MATCH_TEXT : 0);

							if(dwFlags & MATCH_NICKNAME)
								dwFlags |= (::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTNICKUID) ? MATCH_UIN : 0);

							M->WriteDword("Chat", "HighlightEnabled", dwFlags);
							g_Settings.Highlight->init();
						}
						return TRUE;
					}
				default:
					break;
			}
			break;
	}
	return(FALSE);
}

INT_PTR CALLBACK CMUCHighlight::dlgProcAdd(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UINT	uCmd = ::GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch(msg) {
		case WM_INITDIALOG: {
			HFONT hFont = (HFONT)::SendDlgItemMessage(hwndDlg, IDC_ADDHIGHLIGHTTITLE, WM_GETFONT, 0, 0);
			LOGFONT lf = {0};

			THighLightEdit *the = reinterpret_cast<THighLightEdit *>(lParam);
			::SetWindowLongPtr(hwndDlg, GWLP_USERDATA, the->uCmd);

			::GetObject(hFont, sizeof(lf), &lf);
			lf.lfWeight = FW_BOLD;
			lf.lfHeight = (int)(lf.lfHeight * 1.2);
			hFont = ::CreateFontIndirect(&lf);

			::SendDlgItemMessage(hwndDlg, IDC_ADDHIGHLIGHTTITLE, WM_SETFONT, (WPARAM)hFont, FALSE);
			if(the->uCmd == THighLightEdit::CMD_ADD) {
				::ShowWindow(GetDlgItem(hwndDlg, IDC_ADDHIGHLIGHTEDITLIST), SW_HIDE);
				::SetDlgItemText(hwndDlg, IDC_ADDHIGHLIGHTTITLE, CTranslator::get(CTranslator::GEN_MUC_HIGHLIGHT_ADD));
				::SendDlgItemMessage(hwndDlg, IDC_ADDHIGHLIGHTNAME, CB_INSERTSTRING, -1, (LPARAM)the->ui->pszNick);
				::SendDlgItemMessage(hwndDlg, IDC_ADDHIGHLIGHTNAME, CB_INSERTSTRING, -1, (LPARAM)the->ui->pszUID);
				::SendDlgItemMessage(hwndDlg, IDC_ADDHIGHLIGHTNAME, CB_SETCURSEL, 1, 0);
			} else {
				::ShowWindow(GetDlgItem(hwndDlg, IDC_ADDHIGHLIGHTNAME), SW_HIDE);
				::ShowWindow(GetDlgItem(hwndDlg, IDC_ADDHIGHLIGHTEXPLAIN), SW_HIDE);
				::SetDlgItemText(hwndDlg, IDC_ADDHIGHLIGHTTITLE, CTranslator::get(CTranslator::GEN_MUC_HIGHLIGHT_EDIT));
			}


			break;
		}

		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			HWND hwndChild = (HWND)lParam;
			UINT id = ::GetDlgCtrlID(hwndChild);

			if(hwndChild == ::GetDlgItem(hwndDlg, IDC_ADDHIGHLIGHTTITLE))
				::SetTextColor((HDC)wParam, RGB(60, 60, 150));
			::SetBkColor((HDC)wParam, ::GetSysColor(COLOR_WINDOW));
			return (INT_PTR)::GetSysColorBrush(COLOR_WINDOW);
		}

		case WM_COMMAND: {
			if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			   ::DestroyWindow(hwndDlg);
			break;
		}

		case WM_DESTROY: {
			HFONT hFont = (HFONT)::SendDlgItemMessage(hwndDlg, IDC_ADDHIGHLIGHTTITLE, WM_GETFONT, 0, 0);
			::DeleteObject(hFont);
			break;
		}
	}
	return(FALSE);
}

