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
#include <vector>
#include <set>
#include <fstream>

extern "C" int _DebugPopup(HANDLE hContact, const char *fmt, ...);

#if defined(UNICODE)

/* 
 * old code (textformat plugin dealing directly in the edit control - not the best solution, but the author
 * had no other choice as srmm never had an api for this...
 */

static WCHAR *w_bbcodes_begin[] = { L"[b]", L"[i]", L"[u]" };
static WCHAR *w_bbcodes_end[] = { L"[/b]", L"[/i]", L"[/u]" };

static WCHAR *formatting_strings_begin[] = { L"%b1 ", L"%i1 ", L"%u1 " };
static WCHAR *formatting_strings_end[] = { L"%b0 ", L"%i0 ", L"%u0 " };

#define NR_CODES 3
/*
 * this translates formatting tags into rtf sequences...
 * if bWordsOnly is != 0, then only formatting tags 
 */

extern "C" const WCHAR *FormatRaw(const WCHAR *msg, int bWordsOnly)
{
    static std::wstring message(msg);
    unsigned beginmark = 0, endmark = 0, tempmark = 0, index;
    int i, endindex;
    WCHAR endmarker;
    message.assign(msg);

    if(message.find(L"://") != message.npos)
       return(message.c_str());
#ifdef __MATHMOD_SUPPORT
    if(myGlobals.m_MathModAvail && message.find(myGlobals.m_MathModStartDelimiter) != message.npos)
        return(message.c_str());
#endif
    /*
     * always do bbcode
     */

    while(TRUE) {
        for(i = 0; i < NR_CODES; i++) {
            if((tempmark = message.find(w_bbcodes_begin[i], beginmark)) != message.npos)
                break;
        }
        if(i >= NR_CODES)
            break;

        beginmark = tempmark;
        endindex = i;
        if((endmark = message.find(w_bbcodes_end[i], beginmark)) == message.npos)
            break;
        message.replace(endmark, 4, formatting_strings_end[i]);
        message.insert(beginmark, L" ");
        message.replace(beginmark, 4, formatting_strings_begin[i]);
    }
    
    while((beginmark = message.find_first_of(L"*/_", beginmark)) != message.npos) {
        endmarker = message[beginmark];
        if(bWordsOnly) {
            if(beginmark > 0 && !iswspace(message[beginmark - 1]) && !iswpunct(message[beginmark - 1])) {
                beginmark++;
                continue;
            }
            // search a corresponding endmarker which fulfills the criteria
            unsigned tempmark = beginmark + 1;
            while((endmark = message.find(endmarker, tempmark)) != message.npos) {
                if(iswpunct(message[endmark + 1]) || iswspace(message[endmark + 1]) || message[endmark + 1] == 0 || wcschr(L"*/_", message[endmark + 1]) != NULL)
                    goto ok;
                tempmark = endmark + 1;
            }
            break;
        }
        else {
            if((endmark = message.find(endmarker, beginmark + 1)) == message.npos)
                break;
        }
ok:        
        if((endmark - beginmark) < 2) {
            beginmark++;
            continue;
        }
        index = 0;
        switch(endmarker) {
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
        message.insert(endmark, L"%%%");
        message.replace(endmark, 4, formatting_strings_end[index]);
        message.insert(beginmark, L"%%%");
        message.replace(beginmark, 4, formatting_strings_begin[index]);
    }
    //MessageBoxW(0, message.c_str(), L"foo", MB_OK);
    return(message.c_str());
}

#else   // unicode

/* this is the ansi version */
static char *bbcodes_begin[] = { "[b]", "[i]", "[u]" };
static char *bbcodes_end[] = { "[/b]", "[/i]", "[/u]" };

#define NR_CODES 3

static char *formatting_strings_begin[] = { "%b1 ", "%i1 ", "%u1 " };
static char *formatting_strings_end[] = { "%b0 ", "%i0 ", "%u0 " };

/*
 * this translates formatting tags into rtf sequences...
 */

extern "C" const char *FormatRaw(const char *msg, int bWordsOnly)
{
    static std::string message(msg);
    unsigned beginmark = 0, endmark = 0, tempmark = 0, index;
    char endmarker;
    int i, endindex;
    message.assign(msg);

    if(message.find("://") != message.npos)
       return(message.c_str());
#ifdef __MATHMOD_SUPPORT
    if(myGlobals.m_MathModAvail && message.find(myGlobals.m_MathModStartDelimiter) != message.npos)
        return(message.c_str());
#endif    

    /*
     * always do bbcode
     */

    while(TRUE) {
        for(i = 0; i < NR_CODES; i++) {
            if((tempmark = message.find(bbcodes_begin[i], beginmark)) != message.npos)
                break;
        }
        if(i >= NR_CODES)
            break;

        beginmark = tempmark;
        endindex = i;
        if((endmark = message.find(bbcodes_end[i], beginmark)) == message.npos)
            break;
        message.replace(endmark, 4, formatting_strings_end[i]);
        message.insert(beginmark, " ");
        message.replace(beginmark, 4, formatting_strings_begin[i]);
    }

    while((beginmark = message.find_first_of("*/_", beginmark)) != message.npos) {
        endmarker = message[beginmark];
        if(bWordsOnly) {
            if(beginmark > 0 && !isspace(message[beginmark - 1]) && !ispunct(message[beginmark - 1])) {
                beginmark++;
                continue;
            }
            // search a corresponding endmarker which fulfills the criteria
            unsigned tempmark = beginmark + 1;
            while((endmark = message.find(endmarker, tempmark)) != message.npos) {
                if(ispunct(message[endmark + 1]) || isspace(message[endmark + 1]) || message[endmark + 1] == 0 || strchr("*/_", message[endmark + 1]) != NULL)
                    goto ok;
                tempmark = endmark + 1;
            }
            break;
        }
        else {
            if((endmark = message.find(endmarker, beginmark + 1)) == message.npos)
                break;
        }
ok:        
        if((endmark - beginmark) < 2) {
            beginmark++;
            continue;
        }
        index = 0;
        switch(endmarker) {
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
        message.insert(endmark, "%%%");
        message.replace(endmark, 4, formatting_strings_end[index]);
        message.insert(beginmark, "%%%");
        message.replace(beginmark, 4, formatting_strings_begin[index]);
    }
    return(message.c_str());
}

#endif

