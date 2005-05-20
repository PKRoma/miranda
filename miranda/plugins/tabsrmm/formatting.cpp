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
#include "msgdlgutils.h"

#define MWF_LOG_TEXTFORMAT 0x2000000

extern "C" RTFColorTable rtf_ctable[];

extern "C" int _DebugPopup(HANDLE hContact, const char *fmt, ...);

#if defined(UNICODE)

/* 
 * old code (textformat plugin dealing directly in the edit control - not the best solution, but the author
 * had no other choice as srmm never had an api for this...
 */

static WCHAR *w_bbcodes_begin[] = { L"[b]", L"[i]", L"[u]", L"[color=" };
static WCHAR *w_bbcodes_end[] = { L"[/b]", L"[/i]", L"[/u]", L"[/color]" };

static WCHAR *formatting_strings_begin[] = { L"b1 ", L"i1 ", L"u1 ", L"c1 " };
static WCHAR *formatting_strings_end[] = { L"b0 ", L"i0 ", L"u0 ", L"c0 " };

#define NR_CODES 4
/*
 * this translates formatting tags into rtf sequences...
 * flags: loword = words only for simple  * /_ formatting
 *        hiword = bbcode support (strip bbcodes if 0)
 */

extern "C" const WCHAR *FormatRaw(DWORD dwFlags, const WCHAR *msg, int flags)
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

    if(HIWORD(flags) == 0)
        goto nobbcode;
    
    beginmark = 0;
    while(TRUE) {
        for(i = 0; i < NR_CODES; i++) {
            if((tempmark = message.find(w_bbcodes_begin[i], 0)) != message.npos)
                break;
        }
        if(i >= NR_CODES)
            break;
        beginmark = tempmark;
        endindex = i;
        if((endmark = message.find(w_bbcodes_end[i], beginmark)) == message.npos)
            break;
        if(endindex == 3) {                                  // color
            int closing = message.find_first_of(L"]", beginmark);
            if(closing == message.npos)
                continue;                                       // no closing bracket found
            else {
                //std::wstring colorname = message.substr(beginmark + 7, closing - (beginmark + 7));
                std::wstring colorname = message.substr(beginmark + 7, 10);
                int ii = 0;
                wchar_t szTemp[5];
                while(rtf_ctable[ii].szName != NULL) {
//                    if(colorname == rtf_ctable[ii].szName) {
                    if(colorname.find(rtf_ctable[ii].szName, 0) != colorname.npos) {
                        closing = beginmark + 7 + wcslen(rtf_ctable[ii].szName);
                        message.erase(endmark, 4);
                        message.replace(endmark, 4, L"c0 ");
                        message.erase(beginmark, (closing - beginmark));
                        message.insert(beginmark, L"cxxx ");
                        _snwprintf(szTemp, 4, L"%02d", MSGDLGFONTCOUNT + 10 + ii);
                        message[beginmark + 3] = szTemp[0];
                        message[beginmark + 4] = szTemp[1];
                        break;
                    }
                    ii++;
                }
                if(rtf_ctable[ii].szName == NULL) {
                    message.erase(endmark, 8);
                    message.erase(beginmark, (closing - beginmark) + 1);
                }
                continue;
            }
        }
        message.replace(endmark, 4, formatting_strings_end[i]);
        message.insert(beginmark, L" ");
        message.replace(beginmark, 4, formatting_strings_begin[i]);
    }
nobbcode:
    if(!(dwFlags & MWF_LOG_TEXTFORMAT))
        goto nosimpletags;
    
    while((beginmark = message.find_first_of(L"*/_", beginmark)) != message.npos) {
        endmarker = message[beginmark];
        if(LOWORD(flags)) {
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
nosimpletags:
    return(message.c_str());
}

#else   // unicode

/* this is the ansi version */
static char *bbcodes_begin[] = { "[b]", "[i]", "[u]", "[color=" };
static char *bbcodes_end[] = { "[/b]", "[/i]", "[/u]", "[/color]" };

#define NR_CODES 4

static char *formatting_strings_begin[] = { "b1 ", "i1 ", "u1 ", "c1 " };
static char *formatting_strings_end[] = { "b0 ", "i0 ", "u0 ", "c0 " };

/*
 * this translates formatting tags into rtf sequences...
 */

extern "C" const char *FormatRaw(DWORD dwFlags, const char *msg, int flags)
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

    if(HIWORD(flags) == 0)
        goto nobbcode;
    
    while(TRUE) {
        for(i = 0; i < NR_CODES; i++) {
            if((tempmark = message.find(bbcodes_begin[i], 0)) != message.npos)
                break;
        }
        if(i >= NR_CODES)
            break;

        beginmark = tempmark;
        endindex = i;
        if((endmark = message.find(bbcodes_end[i], beginmark)) == message.npos)
            break;
        if(endindex == 3) {                                  // color
            int closing = message.find_first_of("]", beginmark);
            if(closing == message.npos)
                continue;                                       // no closing bracket found
            else {
                std::string colorname = message.substr(beginmark + 7, 10);
                //std::string colorname = message.substr(beginmark + 7, closing - (beginmark + 7));
                int ii = 0;
                char szTemp[5];
                while(rtf_ctable[ii].szName != NULL) {
                    //if(colorname == rtf_ctable[ii].szName) {
                    if(colorname.find(rtf_ctable[ii].szName, 0) != colorname.npos) {
                        closing = beginmark + 7 + strlen(rtf_ctable[ii].szName);
                        message.erase(endmark, 8);
                        message.insert(endmark, "c0xx ");
                        message.erase(beginmark, (closing - beginmark) + 1);
                        message.insert(beginmark, "cxxx ");
                        _snprintf(szTemp, 4, "%02d", MSGDLGFONTCOUNT + 10 + ii);
                        message[beginmark + 3] = szTemp[0];
                        message[beginmark + 4] = szTemp[1];
                        break;
                    }
                    ii++;
                }
                if(rtf_ctable[ii].szName == NULL) {
                    message.erase(endmark, 8);
                    message.erase(beginmark, (closing - beginmark) + 1);
                }
                continue;
            }
        }
        message.replace(endmark, 4, formatting_strings_end[i]);
        message.insert(beginmark, " ");
        message.replace(beginmark, 4, formatting_strings_begin[i]);
    }
    
nobbcode:
    if(!(dwFlags & MWF_LOG_TEXTFORMAT))            // skip */_ stuff if not enabled
        goto nosimpletags;
    
    while((beginmark = message.find_first_of("*/_", beginmark)) != message.npos) {
        endmarker = message[beginmark];
        if(LOWORD(flags)) {
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
nosimpletags:
    return(message.c_str());
}

#endif

