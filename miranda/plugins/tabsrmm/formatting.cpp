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

simple text formatting routines, taken from the textformat plugin for Miranda

-- Miranda TextFormat Plugin
-- Copyright (C) 2003 Daniel Wesslén (wesslen)

Modified and adapted for tabSRMM by Nightwish
Unicode version by Nightwish

License: GPL
*/

#if defined(UNICODE)

#define __TSR_CXX
#include "commonheaders.h"

#include <windows.h>
#include <richedit.h>

#include <string>
#include <vector>
#include <set>
#include <fstream>

extern "C" int _DebugPopup(HANDLE hContact, const char *fmt, ...);
                     
unsigned FormatSpan(HWND REdit, const std::wstring &text, unsigned npos, wchar_t prech)
{
	wchar_t empty[] = L"";

	// skip escaped section, remove the backslash
	if (prech == '\\') {
		SendMessage(REdit, EM_SETSEL, npos-1, npos);
		SendMessageA(REdit, EM_REPLACESEL, FALSE, (LPARAM) empty);
		return text.size() - 1;
	}

	std::wstring new_text = text, stripped_chars;
	unsigned osize = text.size();

	CHARFORMAT fmt;
	fmt.cbSize = sizeof fmt;
	fmt.dwMask = 0;
	fmt.dwEffects = 0;

	while (new_text.size() >= 3 && wcschr(L"_*/", new_text[0])
		&& new_text[0] == new_text[new_text.size()-1]) {

		wchar_t ch = new_text[0];
		stripped_chars += ch;

		new_text.erase(0, 1);
		new_text.erase(new_text.size()-1);

		if (ch == '*') {
			fmt.dwMask |= CFM_BOLD;
			fmt.dwEffects |= CFE_BOLD;
		} else if (ch == '/') {
			fmt.dwMask |= CFM_ITALIC;
			fmt.dwEffects |= CFE_ITALIC;
		} else if (ch == '_') {
			fmt.dwMask |= CFM_UNDERLINE;
			fmt.dwEffects |= CFE_UNDERLINE;
		}
	}

	SendMessage(REdit, EM_SETSEL, npos+osize-stripped_chars.size(), npos+osize);
	SendMessage(REdit, EM_REPLACESEL, FALSE, (LPARAM) empty);
	SendMessage(REdit, EM_SETSEL, npos, npos+stripped_chars.size());
	SendMessage(REdit, EM_REPLACESEL, FALSE, (LPARAM) empty);

	unsigned p, pos = 0;

    while ((p = new_text.find_first_of(stripped_chars)) != new_text.npos) {
		new_text[p] = ' ';
		wchar_t str[] = L" ";
		SendMessage(REdit, EM_SETSEL, npos+p, npos+p+1);
		SendMessage(REdit, EM_REPLACESEL, FALSE, (LPARAM) str);
	}

	SendMessage(REdit, EM_SETSEL, npos, npos + new_text.size());
	SendMessage(REdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &fmt);

	return new_text.size();
}

extern "C" int FormatText(HWND REdit, unsigned npos, unsigned maxlength)
{
	GETTEXTLENGTHEX gtl;
    GETTEXTEX gtx = {0};
    gtl.codepage = 1200;
    gtl.flags = GTL_DEFAULT | GTL_PRECISE | GTL_NUMCHARS;
	int textlen = SendMessage(REdit, EM_GETTEXTLENGTHEX, (WPARAM) &gtl, 0);
	//if (textlen > (int)maxlength)
	//	textlen = (int)maxlength;
	int nleft = textlen-npos;
	if (nleft <= 0) // _should_ never happen
		return -1;
	std::vector<wchar_t> buf(nleft+1);
	TEXTRANGE tr;
	tr.lpstrText = &buf[0];
	tr.chrg.cpMin = npos;
	tr.chrg.cpMax = textlen + 1;

    gtx.cb = (nleft + 1) * sizeof(wchar_t);
    gtx.codepage = 1200;
    gtx.flags = 2;
    gtx.lpDefaultChar = 0;
    gtx.lpUsedDefChar = 0;

    SendMessageW(REdit, EM_EXSETSEL, 0, (LPARAM)&tr.chrg);
    SendMessageW(REdit, EM_GETTEXTEX, (WPARAM)&gtx, (LPARAM)&buf[0]);
    
    //SendMessageW(REdit, EM_GETTEXTRANGE, 0, (LPARAM) &tr);
    //MessageBoxW(0, &buf[0], L"foo", 0);

	std::wstring old_text(buf.begin(), buf.end()-1);

	const wchar_t * const pre_chars = L" \t\n\r\"'!?;:,.-=()[]{}@$%&#\\<>|+^~";
	const wchar_t * const post_chars = pre_chars;
#define invalidating_chars L"\01 \r\n\t"

	unsigned p, opos = 0;
	while ((p = old_text.find_first_of(L"/_*", opos)) != old_text.npos) {

		std::wstring t_(old_text.c_str(), p);
		const wchar_t *what = t_.c_str();
        bool space_seen = false, sep_seen = false;
        unsigned searchoffs = p, epos;
        wchar_t searchfor[] = invalidating_chars;

		wchar_t sep = old_text[p];

		// can't start in the last two chars
		if (p >= old_text.size()-2)
			break;

		// check preceeding char
		if (p != 0 && !(wcschr(pre_chars, old_text[p-1]) || !_istprint(old_text[p-1])))
			goto miss;

		// search for end
		searchfor[0] = sep;
//		int countdown = 50;
		for (;; searchoffs = epos) {
//			if (!--countdown)
//				goto miss;

			epos = old_text.find_first_of(searchfor, searchoffs+1);
			if (epos == old_text.npos)
				goto miss;

			wchar_t found = old_text[epos];

			if (found == sep || found == ' ') {
				if (epos == old_text.size()-1) // end of stream
					goto ok;
				wchar_t next = old_text[epos+1];
				if (found == sep && wcschr(post_chars, next)) // single word or end of sequence
					goto ok;

				if (found == sep) {
//					countdown = 10;
					sep_seen = true;
					if (space_seen)
						goto miss;
					continue;
				} else if (found == ' ') {
					space_seen = true;
					if (sep_seen)
						goto miss;
					continue;
				} else
					goto miss;
			} else
				goto miss;
		}

	ok: {
		std::wstring oldsub(old_text, p, epos-p+1);

		unsigned first = oldsub.find_first_not_of(L"_*/");
		if (first == oldsub.npos || oldsub[first] == ' ')
			goto miss;
		unsigned last = oldsub.find_last_not_of(L"_*/");
		if (last == oldsub.npos || oldsub[last] == ' ')
			goto miss;

		if (oldsub.find(L"**") != oldsub.npos)
			goto miss;
		if (oldsub.find(L"__") != oldsub.npos)
			goto miss;
		if (oldsub.find(L"//") != oldsub.npos)
			goto miss;
		unsigned skipped = p - opos;

		//if (g_ignores.count(oldsub))
		//	goto miss;

		npos += skipped;
		unsigned newlen;
		if (p == 0)
			newlen = FormatSpan(REdit, oldsub, npos, ' ');
		else
			newlen = FormatSpan(REdit, oldsub, npos, old_text[p-1]);

		npos += newlen;
		opos += skipped + oldsub.size();
		continue;
		}
	miss: {
		unsigned skipped = p - opos;
		npos += skipped+1;
		opos += skipped+1;
		}
	}

	npos += old_text.size() - opos;

	return npos;
}

#endif // _UNICODE

