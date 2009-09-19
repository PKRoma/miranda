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

// #define __HLT_PERFSTATS 1

void CMUCHighlight::cleanup()
{
	if(m_NickPatternString)
		mir_free(m_NickPatternString);
	if(m_TextPatternString)
		mir_free(m_TextPatternString);

	m_TextPatternString = m_NickPatternString = 0;

	if(m_NickPatterns)
		mir_free(m_NickPatterns);
	if(m_TextPatterns)
		mir_free(m_TextPatterns);

	m_iNickPatterns = m_iTextPatterns = 0;
	m_NickPatterns = m_TextPatterns = 0;
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

	tokenize(m_TextPatternString, m_TextPatterns, m_iTextPatterns);
	tokenize(m_NickPatternString, m_NickPatterns, m_iNickPatterns);

	/*for(int i = 0; i < m_iTextPatterns; i++)
		_DebugTraceW(_T("textpattern: %s %c, %c"), m_TextPatterns[i], *(m_TextPatterns[i]), *((m_TextPatterns[i] + 1)));

	for(i = 0; i < m_iNickPatterns; i++)
		_DebugTraceW(_T("NICKpattern: %s"), m_NickPatterns[i]);*/
}

void CMUCHighlight::tokenize(TCHAR *tszString, TCHAR**& patterns, UINT& nr)
{
	if(tszString == 0)
		return;

	TCHAR	*p = tszString;

	if(*p == 0)
		return;

	nr = 0;

	if(*p != ' ')
		nr++;

	while(*p) {
		if(*p == ' ') {
			p++;
			while(*p && _istspace(*p))
				p++;
			if(*p)
				nr++;
		}
		p++;
	}
	patterns = (TCHAR **)mir_alloc(nr * sizeof(TCHAR *));

	p = tszString;
	nr = 0;

	if(*p != ' ')
		patterns[nr++] = p;

	while(*p) {
		if(*p == ' ') {
			*p++ = 0;
			while(*p && _istspace(*p))
				p++;
			if(*p)
				patterns[nr++] = p;
		}
		p++;
	}
}

int CMUCHighlight::match(const GCEVENT *pgce, const SESSION_INFO *psi, DWORD dwFlags, const TCHAR *tszAdditonal)
{
	int 			result = 0, nResult = 0;

	if(pgce == 0)
		return(0);

	if((m_dwFlags & MATCH_TEXT) && (dwFlags & MATCH_TEXT) && m_iTextPatterns > 0) {
#ifdef __HLT_PERFSTATS
		int		words = 0;
		M->startTimer();
#endif
		TCHAR	*tszCleaned = ::RemoveFormatting(pgce->ptszText, true);
		TCHAR	*p = tszCleaned;
		TCHAR  	*p1;
		UINT	i = 0;

		TCHAR	*tszMe = mir_tstrdup(psi->pMe->pszNick);
		_tcslwr(tszMe);

		while(p && !result) {
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

				for(i = 0; i < m_iTextPatterns && !result; i++) {
					if(*(m_TextPatterns[i]) == '%' && *((m_TextPatterns[i]) + 1)  == 'm') {
						result = wildmatch(tszMe, p) ? MATCH_TEXT : 0;
					} else
						result = wildmatch(m_TextPatterns[i], p) ? MATCH_TEXT : 0;
				}

				if(p1) {
					*p1 = ' ';
					p = p1 + 1;
				}
				else
					p = 0;
#ifdef __HLT_PERFSTATS
				words++;
#endif
			}
			else
				break;
		}

#ifdef __HLT_PERFSTATS
		M->stopTimer(0);
		if(psi && psi->dat) {
			mir_sntprintf(psi->dat->szStatusBar, 100, _T("PERF text match: %d ticks = %f msec (%d words, %d patterns)"), (int)M->getTicks(), M->getMsec(), words, m_iTextPatterns);
			if(psi->dat->pContainer->hwndStatus)
				::SendMessage(psi->dat->pContainer->hwndStatus, SB_SETTEXT, 0, (LPARAM)psi->dat->szStatusBar);
		}
#endif
		mir_free(tszMe);
	}

	/*
	 * optinally, match the nickname against the list of nicks to highlight
	 */
	if((m_dwFlags & MATCH_NICKNAME) && (dwFlags & MATCH_NICKNAME) && pgce->ptszNick && m_iNickPatterns > 0) {

		for(UINT i = 0; i < m_iNickPatterns && !nResult; i++) {
			nResult = wildmatch(m_NickPatterns[i], pgce->ptszNick) ? MATCH_NICKNAME : 0;
			if(tszAdditonal) {
				nResult = wildmatch(m_NickPatterns[i], tszAdditonal) ? MATCH_NICKNAME : 0;
				//_DebugTraceW(_T("match %s against additional: %s (%d)"), *it, tszAdditonal, nResult);
			}
			if((m_dwFlags & MATCH_UIN) && pgce->ptszUserInfo) {
				nResult = wildmatch(m_NickPatterns[i], pgce->ptszUserInfo) ? MATCH_NICKNAME : 0;
				//_DebugTraceW(_T("match %s against UID: %s (%d)"), *it, pgce->ptszUserInfo, nResult);
			}
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

