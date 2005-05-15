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
GNU General Public License for more details .

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

$Id$
*/

#include "commonheaders.h"
#pragma hdrstop
#include <ctype.h>
#include <malloc.h>
#include <mbstring.h>
#include <time.h>
#include <locale.h>
#include "msgs.h"
#include "m_smileyadd.h"
// IEVIew MOD Begin
#include "m_ieview.h"
#include "m_popup.h"
#include "nen.h"
#include "functions.h"
// IEVIew MOD End
#ifdef __MATHMOD_SUPPORT
//mathMod begin
#include "m_MathModule.h"
//mathMod end
#endif
#include "msgdlgutils.h"

struct CPTABLE cpTable[] = {
    {	874,	"Thai" },
    {	932,	"Japanese" },
    {	936,	"Simplified Chinese" },
    {	949,	"Korean" },
    {	950,	"Traditional Chinese" },
    {	1250,	"Central European" },
    {	1251,	"Cyrillic" },
    {	1252,	"Latin I" },
    {	1253,	"Greek" },
    {	1254,	"Turkish" },
    {	1255,	"Hebrew" },
    {	1256,	"Arabic" },
    {	1257,	"Baltic" },
    {	1258,	"Vietnamese" },
    {	1361,	"Korean (Johab)" },
    {   -1,     NULL}
};

// #define _CACHED_ICONS 1

int _log(const char *fmt, ...);

static char *Template_MakeRelativeDate(struct MessageWindowData *dat, time_t check, int groupBreak, TCHAR code);
static void ReplaceIcons(HWND hwndDlg, struct MessageWindowData *dat, LONG startAt, int fAppend);

static char *weekDays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static char *months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
static char weekDays_translated[7][30];
static char months_translated[12][30];

char szSep0[40], szSep1[152], szSep2[40], szMicroLf[128], szExtraLf[50];
char szMsgPrefixColon[5], szMsgPrefixNoColon[5];
DWORD dwExtraLf = 0;

int g_groupBreak = TRUE;
static char *szMyName, *szYourName;
static char *szDivider = "\\strike-----------------------------------------------------------------------------------------------------------------------------------\\strike0";
static char *szGroupedSeparator = "> ";

extern void ImageDataInsertBitmap(IRichEditOle *ole, HBITMAP hBm);
#if defined(_UNICODE)
    extern WCHAR *FormatRaw(DWORD dwFlags, const WCHAR *msg, int flags);
#else
    extern char *FormatRaw(DWORD dwFlags, const char *msg, int flags);
#endif

extern void ReleaseRichEditOle(IRichEditOle *ole);

extern MYGLOBALS myGlobals;
extern struct RTFColorTable rtf_ctable[];

static int logPixelSY;

static char szToday[22], szYesterday[22];

#define LOGICON_MSG  0
#define LOGICON_URL  1
#define LOGICON_FILE 2
#define LOGICON_OUT 3
#define LOGICON_IN 4
#define LOGICON_STATUS 5
#define LOGICON_ERROR 6

#if defined _CACHED_ICONS
    struct MsgLogIcon msgLogIcons[NR_LOGICONS * 3];
#else
    static HICON Logicons[NR_LOGICONS];
#endif    

#define STREAMSTAGE_HEADER  0
#define STREAMSTAGE_EVENTS  1
#define STREAMSTAGE_TAIL    2
#define STREAMSTAGE_STOP    3
struct LogStreamData
{
    int stage;
    HANDLE hContact;
    HANDLE hDbEvent, hDbEventLast;
    char *buffer;
    int bufferOffset, bufferLen;
    int eventsToInsert;
    int isEmpty;
    struct MessageWindowData *dlgDat;
    DBEVENTINFO *dbei;
};

char rtfFonts[MSGDLGFONTCOUNT + 2][128];

int safe_wcslen(wchar_t *msg, int chars) {
    int i;

    for(i = 0; i < chars; i++) {
        if(msg[i] == (WCHAR)0)
            return i;
    }
    return 0;
}
/*
 * remove any empty line at the end of a message to avoid some RichEdit "issues" with
 * the highlight code (individual background colors).
 * Doesn't touch the message for sure, but empty lines at the end are ugly anyway.
 */

void TrimMessage(TCHAR *msg)
{
    int iLen = _tcslen(msg) - 1;
    int i = iLen;

    while(i && (msg[i] == '\r' || msg[i] == '\n')) {
        i--;
    }
    if(i < iLen)
        msg[i+1] = '\0';
}

void CacheLogFonts()
{
    LOGFONTA lf;
    int i;
    HDC hdc = GetDC(NULL);
    logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);

    for(i = 0; i < MSGDLGFONTCOUNT; i++) {
        LoadMsgDlgFont(i, &lf, NULL);
        wsprintfA(rtfFonts[i], "\\f%u\\cf%u\\b%d\\i%d\\fs%u", i, i, lf.lfWeight >= FW_BOLD ? 1 : 0, lf.lfItalic, 2 * abs(lf.lfHeight) * 74 / logPixelSY);
    }
    wsprintfA(rtfFonts[MSGDLGFONTCOUNT], "\\f%u\\cf%u\\b%d\\i%d\\fs%u", MSGDLGFONTCOUNT, MSGDLGFONTCOUNT, 0, 0, 0);
    
    strncpy(szToday, Translate("Today"), 20);
    strncpy(szYesterday, Translate("Yesterday"), 20);
}

void UncacheMsgLogIcons()
{
#ifdef _CACHED_ICONS
    int i;

    for(i = 0; i < 3 * NR_LOGICONS; i++)
        DeleteCachedIcon(&msgLogIcons[i]);
#endif    
}

/*
 * cache copies of all our msg log icons with 3 background colors to speed up the
 * process of inserting icons into the RichEdit control.
 */

void CacheMsgLogIcons()
{
#ifdef _CACHED_ICONS
    HICON icons[NR_LOGICONS];
    int iCounter = 0;
    int i;
    int size;
    
    icons[0] = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
    icons[1] = LoadSkinnedIcon(SKINICON_EVENT_URL);
    icons[2] = LoadSkinnedIcon(SKINICON_EVENT_FILE);
    icons[3] = myGlobals.g_iconOut;
    icons[4] = myGlobals.g_iconIn;
    icons[5] = myGlobals.g_iconStatus;
    icons[6] = myGlobals.g_iconErr;
    
    for(i = 0; i < NR_LOGICONS; i++) {
        if(icons[i] == myGlobals.g_iconOut || icons[i] == myGlobals.g_iconIn)
            size = 0;
        else
            size = 16;          // force mirandas icons into small mode (16x16 pixels - on some systems, they load with incorrect size..?)
        CacheIconToBMP(&msgLogIcons[iCounter++], icons[i], DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR), size, size);
        CacheIconToBMP(&msgLogIcons[iCounter++], icons[i], DBGetContactSettingDword(NULL, SRMSGMOD_T, "inbg", RGB(255,255,255)), size, size);
        CacheIconToBMP(&msgLogIcons[iCounter++], icons[i], DBGetContactSettingDword(NULL, SRMSGMOD_T, "outbg", RGB(255,255,255)), size, size);
    }
#else
    Logicons[0] = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
    Logicons[1] = LoadSkinnedIcon(SKINICON_EVENT_URL);
    Logicons[2] = LoadSkinnedIcon(SKINICON_EVENT_FILE);
    Logicons[3] = myGlobals.g_iconOut;
    Logicons[4] = myGlobals.g_iconIn;
    Logicons[5] = myGlobals.g_iconStatus;
    Logicons[6] = myGlobals.g_iconErr;
#endif    
}

/*
 * pre-translate day and month names to significantly reduce he number of Translate()
 * service calls while building the message log
 */

void PreTranslateDates()
{
    int i;
    char *szTemp;
    
    for(i = 0; i <= 6; i++) {
        szTemp = Translate(weekDays[i]);
        mir_snprintf(weekDays_translated[i], 28, "%s", szTemp);
    }
    for(i = 0; i <= 11; i++) {
        szTemp = Translate(months[i]);
        mir_snprintf(months_translated[i], 28, "%s", szTemp);
    }
}

static int GetColorIndex(char *rtffont)
{
    char *p;
    
    if((p = strstr(rtffont, "\\cf")) != NULL)
        return atoi(p + 3);
    return 0;
}

static void AppendToBuffer(char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
{
    va_list va;
    int charsDone;

    va_start(va, fmt);
    for (;;) {
        charsDone = mir_vsnprintf(*buffer + *cbBufferEnd, *cbBufferAlloced - *cbBufferEnd, fmt, va);
        if (charsDone >= 0)
            break;
        *cbBufferAlloced += 1024;
        *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
    }
    va_end(va);
    *cbBufferEnd += charsDone;
}

#if defined( _UNICODE )

static int AppendUnicodeToBuffer(char **buffer, int *cbBufferEnd, int *cbBufferAlloced, TCHAR * line, int mode)
{
    DWORD textCharsCount = 0;
    char *d;

    int lineLen = wcslen(line) * 9 + 8;
    if (*cbBufferEnd + lineLen > *cbBufferAlloced) {
        cbBufferAlloced[0] += (lineLen + 1024 - lineLen % 1024);
        *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
    }

    d = *buffer + *cbBufferEnd;
    strcpy(d, "{\\uc1 ");
    d += 6;

    for (; *line; line++, textCharsCount++) {
        
        if(1) {
            if(*line == '%' && line[1] != 0) {
                TCHAR code = line[2];
                if(((code == '0' || code == '1') && line[3] == ' ') || (line[1] == 'c' && code == 'x')){
                    int begin = (code == '1');
                    switch(line[1]) {
                        case 'b':
                            CopyMemory(d, begin ? "\\b " : "\\b0 ", begin ? 3 : 4);
                            d += (begin ? 3 : 4);
                            line += 3;
                            continue;
                        case 'i':
                            CopyMemory(d, begin ? "\\i " : "\\i0 ", begin ? 3 : 4);
                            d += (begin ? 3 : 4);
                            line += 3;
                            continue;
                        case 'u':
                            CopyMemory(d, begin ? "\\ul " : "\\ul0 ", begin ? 4 : 5);
                            d += (begin ? 4 : 5);
                            line += 3;
                            continue;
                        case 'c':
                            begin = (code == 'x');
                            CopyMemory(d, "\\cf", 3);
                            if(begin) {
                                d[3] = (char)line[3];
                                d[4] = (char)line[4];
                                d[5] = ' ';
                            }
                            else {
                                char szTemp[10];
                                int colindex = GetColorIndex(rtfFonts[LOWORD(mode) ? (MSGFONTID_MYMSG + (HIWORD(mode) ? 8 : 0)) : (MSGFONTID_YOURMSG + (HIWORD(mode) ? 8 : 0))]);
                                _snprintf(szTemp, 4, "%02d", colindex);
                                d[3] = szTemp[0];
                                d[4] = szTemp[1];
                                d[5] = ' ';
                            }
                            d += 6;
                            line += (begin ? 6 : 3);
                            continue;
                    }
                }
            }
        }
        if (*line == '\r' && line[1] == '\n') {
			CopyMemory(d,"\\line ",6);
			line++;
			d += 6;
        }
        else if (*line == '\n') {
            CopyMemory(d, "\\line ", 6);
            d += 6;
        }
        else if (*line == '\t') {
            CopyMemory(d, "\\tab ", 5);
            d += 5;
        }
        else if (*line == '\\' || *line == '{' || *line == '}') {
            *d++ = '\\';
            *d++ = (char) *line;
        }
        else if (*line < 128) {
            *d++ = (char) *line;
        }
        else
            d += sprintf(d, "\\u%d ?", *line);
    }

    strcpy(d, "}");
    d++;

    *cbBufferEnd = (int) (d - *buffer);
    return textCharsCount;
}
#endif

//same as above but does "\r\n"->"\\par " and "\t"->"\\tab " too
static int AppendToBufferWithRTF(int mode, char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
{
    va_list va;
    int charsDone, i;

    va_start(va, fmt);
    for (;;) {
        charsDone = mir_vsnprintf(*buffer + *cbBufferEnd, *cbBufferAlloced - *cbBufferEnd, fmt, va);
        if (charsDone >= 0)
            break;
        *cbBufferAlloced += 1024;
        *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
    }
    va_end(va);
    *cbBufferEnd += charsDone;
    for (i = *cbBufferEnd - charsDone; (*buffer)[i]; i++) {

        if(1) {
            if((*buffer)[i] == '%' && (*buffer)[i + 1] != 0) {
                char code = (*buffer)[i + 2];
                char tag = (*buffer)[i + 1];
                
                if(((code == '0' || code == '1') && (*buffer)[i + 3] == ' ') || (tag == 'c' && (code == 'x' || code == '0'))) {
                    int begin = (code == '1');

                    if (*cbBufferEnd + 5 > *cbBufferAlloced) {
                        *cbBufferAlloced += 1024;
                        *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
                    }
                    switch(tag) {
                        case 'b':
                            CopyMemory(*buffer + i, begin ? "\\b1 " : "\\b0 ", 4);
                            continue;
                        case 'i':
                            CopyMemory(*buffer + i, begin ? "\\i1 " : "\\i0 ", 4);
                            continue;
                        case 'u':
                            MoveMemory(*buffer + i + 2, *buffer + i + 1, *cbBufferEnd - i);
                            CopyMemory(*buffer + i, begin ? "\\ul1 " : "\\ul0 ", 5);
                            *cbBufferEnd += 1;
                            continue;
                        case 'c':
                            begin = (code == 'x');
                            CopyMemory(*buffer + i, "\\cf", 3);
                            if(begin) {
                            }
                            else {
                                char szTemp[10];
                                int colindex = GetColorIndex(rtfFonts[LOWORD(mode) ? (MSGFONTID_MYMSG + (HIWORD(mode) ? 8 : 0)) : (MSGFONTID_YOURMSG + (HIWORD(mode) ? 8 : 0))]);
                                _snprintf(szTemp, 4, "%02d", colindex);
                                (*buffer)[i + 3] = szTemp[0];
                                (*buffer)[i + 4] = szTemp[1];
                            }
                            continue;
                    }
                }
            }
        }
        
        if ((*buffer)[i] == '\r' && (*buffer)[i + 1] == '\n') {
            if (*cbBufferEnd + 5 > *cbBufferAlloced) {
                *cbBufferAlloced += 1024;
                *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
            }
            MoveMemory(*buffer + i + 6, *buffer +i + 2, *cbBufferEnd - i - 1);
            CopyMemory(*buffer+i,"\\line ",6);
            *cbBufferEnd+=4;
        }
        else if ((*buffer)[i] == '\n') {
            if (*cbBufferEnd + 6 > *cbBufferAlloced) {
                *cbBufferAlloced += 1024;
                *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
            }
			MoveMemory(*buffer + i + 6, *buffer + i + 1, *cbBufferEnd - i);
            CopyMemory(*buffer + i, "\\line ", 6);
            *cbBufferEnd += 5;
        }
        else if ((*buffer)[i] == '\t') {
            if (*cbBufferEnd + 5 > *cbBufferAlloced) {
                *cbBufferAlloced += 1024;
                *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
            }
            MoveMemory(*buffer + i + 5, *buffer + i + 1, *cbBufferEnd - i);
            CopyMemory(*buffer + i, "\\tab ", 5);
            *cbBufferEnd += 4;
        }
        else if ((*buffer)[i] == '\\' || (*buffer)[i] == '{' || (*buffer)[i] == '}') {
            if (*cbBufferEnd + 2 > *cbBufferAlloced) {
                *cbBufferAlloced += 1024;
                *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
            }
            MoveMemory(*buffer + i + 1, *buffer + i, *cbBufferEnd - i + 1);
            (*buffer)[i] = '\\';
            ++*cbBufferEnd;
            i++;
        }
    }
    return _mbslen(*buffer + *cbBufferEnd);
}

//free() the return value
static char *CreateRTFHeader(struct MessageWindowData *dat)
{
    char *buffer;
    int bufferAlloced, bufferEnd;
    int i;
    LOGFONTA lf;
    COLORREF colour;

    bufferEnd = 0;
    bufferAlloced = 1024;
    buffer = (char *) malloc(bufferAlloced);
    buffer[0] = '\0';

    // rtl
	if (dat->dwFlags & MWF_LOG_RTL) 
		AppendToBuffer(&buffer,&bufferEnd,&bufferAlloced,"{\\rtf1\\ansi\\deff0\\rtldoc{\\fonttbl");
	else 
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "{\\rtf1\\ansi\\deff0{\\fonttbl");

    for (i = 0; i < MSGDLGFONTCOUNT; i++) {
        LoadMsgDlgFont(i, &lf, NULL);
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "{\\f%u\\fnil\\fcharset%u %s;}", i, lf.lfCharSet, lf.lfFaceName);
    }
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "{\\f%u\\fnil\\fcharset%u %s;}", MSGDLGFONTCOUNT, lf.lfCharSet, "Arial");
    
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "}{\\colortbl ");
    for (i = 0; i < MSGDLGFONTCOUNT; i++) {
        LoadMsgDlgFont(i, NULL, &colour);
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
    }
    if (GetSysColorBrush(COLOR_HOTLIGHT) == NULL)
        colour = RGB(0, 0, 255);
    else
        colour = GetSysColor(COLOR_HOTLIGHT);
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));

    /* OnO: Create incoming and outcoming colours */
    colour = DBGetContactSettingDword(NULL, SRMSGMOD_T, "inbg", RGB(224,224,224));
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
    colour = DBGetContactSettingDword(NULL, SRMSGMOD_T, "outbg", RGB(224,224,224));
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
    colour = DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR);
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
    colour = DBGetContactSettingDword(NULL, SRMSGMOD_T, "hgrid", RGB(224,224,224));
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));

    // custom template colors...

    colour = DBGetContactSettingDword(NULL, SRMSGMOD_T, "cc1", RGB(224,224,224));
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
    colour = DBGetContactSettingDword(NULL, SRMSGMOD_T, "cc2", RGB(224,224,224));
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
    colour = DBGetContactSettingDword(NULL, SRMSGMOD_T, "cc3", RGB(224,224,224));
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
    colour = DBGetContactSettingDword(NULL, SRMSGMOD_T, "cc4", RGB(224,224,224));
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
    colour = DBGetContactSettingDword(NULL, SRMSGMOD_T, "cc5", RGB(224,224,224));
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));

    // bbcode colors...

    i = 0;
    while(rtf_ctable[i].szName != NULL) {
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(rtf_ctable[i].clr), GetGValue(rtf_ctable[i].clr), GetBValue(rtf_ctable[i].clr));
        i++;
    }
        
    // RTL-Support
	if (dat->dwFlags & MWF_LOG_RTL) 
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "}\\rtlpar");
	else 
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "}\\pard");

    // indent
	if(dat->dwFlags & MWF_LOG_INDENT) {
		int iIndent = (int) DBGetContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", 0) * 15;
        int rIndent = (int) DBGetContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", 0) * 15;
        
        if(iIndent) {
            if(dat->dwFlags & MWF_LOG_RTL)
                AppendToBuffer(&buffer,&bufferEnd,&bufferAlloced,"\\ri%u\\fi-%u\\li%u\\tx%u", iIndent + 30, iIndent, rIndent, iIndent + 30);
            else
                AppendToBuffer(&buffer,&bufferEnd,&bufferAlloced,"\\li%u\\fi-%u\\ri%u\\tx%u", iIndent + 30, iIndent, rIndent, iIndent + 30);
        }
	}
    else {
        AppendToBuffer(&buffer,&bufferEnd,&bufferAlloced,"\\li%u\\ri%u\\fi%u", 2*15, 2*15, 0);
    }
    return buffer;
}

//free() the return value
static char *CreateRTFTail(struct MessageWindowData *dat)
{
    char *buffer;
    int bufferAlloced, bufferEnd;

    bufferEnd = 0;
    bufferAlloced = 1024;
    buffer = (char *) malloc(bufferAlloced);
    buffer[0] = '\0';
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "}");
    return buffer;
}

int DbEventIsShown(struct MessageWindowData *dat, DBEVENTINFO * dbei)
{
    switch (dbei->eventType) {
        case EVENTTYPE_MESSAGE:
        case EVENTTYPE_STATUSCHANGE:
            return 1;
        case EVENTTYPE_URL:
            return (dat->dwEventIsShown & MWF_SHOW_URLEVENTS);
        case EVENTTYPE_FILE:
            return(dat->dwEventIsShown & MWF_SHOW_FILEEVENTS);
    }
    return 0;
}

static char *Template_CreateRTFFromDbEvent(struct MessageWindowData *dat, HANDLE hContact, HANDLE hDbEvent, int prefixParaBreak, struct LogStreamData *streamData)
{
    char *buffer, c;
    TCHAR ci, cc;
    char *szName, *szFinalTimestamp, szDummy = '\0';
    int bufferAlloced, bufferEnd, iTemplateLen;
    DBEVENTINFO dbei = { 0 };
    int showColon = 0;
    int isSent = 0;
    int iFontIDOffset = 0, i = 0;
    TCHAR *szTemplate;
    DWORD final_time;
    BOOL skipToNext = FALSE, showTime = TRUE, showDate = TRUE, skipFont = FALSE;
    struct tm event_time;
    TemplateSet *this_templateset;
    BOOL isBold = FALSE, isItalic = FALSE, isUnderline = FALSE;
    
    if(streamData->dbei != 0)
        dbei = *(streamData->dbei);
    else {
        dbei.cbSize = sizeof(dbei);
        dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM) hDbEvent, 0);
        if (dbei.cbBlob == -1)
            return NULL;
        dbei.pBlob = (PBYTE) malloc(dbei.cbBlob);
        CallService(MS_DB_EVENT_GET, (WPARAM) hDbEvent, (LPARAM) & dbei);
        if (!DbEventIsShown(dat, &dbei)) {
            free(dbei.pBlob);
            return NULL;
        }
    }
    dat->stats.lastReceivedChars = 0;
    
    dat->isHistory = (dbei.timestamp < (DWORD)dat->stats.started && (dbei.flags & DBEF_READ || dbei.flags & DBEF_SENT));
    iFontIDOffset = dat->isHistory ? 8 : 0;     // offset into the font table for either history (old) or new events... (# of fonts per configuration set)
    isSent = (dbei.flags & DBEF_SENT);
    
    if(!isSent && (dbei.eventType == EVENTTYPE_STATUSCHANGE || dbei.eventType==EVENTTYPE_MESSAGE || dbei.eventType==EVENTTYPE_URL)) {
		CallService(MS_DB_EVENT_MARKREAD,(WPARAM)hContact,(LPARAM)hDbEvent);
		CallService(MS_CLIST_REMOVEEVENT,(WPARAM)hContact,(LPARAM)hDbEvent);
	}

    bufferEnd = 0;
    bufferAlloced = 1024;
    buffer = (char *) malloc(bufferAlloced);
    buffer[0] = '\0';
    g_groupBreak = TRUE;
    
    if(dat->dwFlags & MWF_DIVIDERWANTED) {
        if(dat->dwFlags & MWF_LOG_INDIVIDUALBKG)
            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\par\\highlight%d", MSGDLGFONTCOUNT + 1 + ((LOWORD(dat->iLastEventType) & DBEF_SENT) ? 1 : 0));
        else
            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\par");
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s\\tab", rtfFonts[H_MSGFONTID_DIVIDERS]);
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szDivider);
        dat->dwFlags &= ~MWF_DIVIDERWANTED;
    }
    if(dat->dwFlags & MWF_LOG_GROUPMODE && dbei.flags == LOWORD(dat->iLastEventType) && dbei.eventType == EVENTTYPE_MESSAGE && HIWORD(dat->iLastEventType) == EVENTTYPE_MESSAGE && ((dbei.timestamp - dat->lastEventTime) < 86400)) {
        g_groupBreak = FALSE;
        if(prefixParaBreak)
            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szSep2);
    }
    else {
        if (prefixParaBreak) {
            if(dwExtraLf)
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szExtraLf, MSGDLGFONTCOUNT + 1 + ((LOWORD(dat->iLastEventType) & DBEF_SENT) ? 1 : 0));
            if(dat->dwFlags & MWF_LOG_GRID) {
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szSep0, rtfFonts[MSGDLGFONTCOUNT]);
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szSep1, MSGDLGFONTCOUNT + 4);
            }
            else
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szSep2);
        }
    }

    /* OnO: highlight start */
    if(dat->dwFlags & MWF_LOG_INDIVIDUALBKG)
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight%d", MSGDLGFONTCOUNT + 1 + ((isSent) ? 1 : 0));
    else if(dat->dwFlags & MWF_LOG_GRID)
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight%d", MSGDLGFONTCOUNT + 3);

    /*
     * templated code starts here
     */

    if (dat->dwFlags & MWF_LOG_SHOWTIME) {
        final_time = dbei.timestamp;
        if (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "uselocaltime", 0)) {
            if(!isSent)
            {
                DWORD local_gmt_diff, contact_gmt_diff;
                int diff;

                time_t now = time(NULL);
                struct tm gmt = *gmtime(&now);
                time_t gmt_time = mktime(&gmt);
                local_gmt_diff=(int)difftime(now, gmt_time);
                contact_gmt_diff=(DWORD)DBGetContactSettingByte(dat->hContact,"UserInfo","Timezone",-1);
                if (contact_gmt_diff==-1) {
                    contact_gmt_diff=(DWORD)DBGetContactSettingByte(dat->hContact, dat->szProto,"Timezone",-1);
                }
                if (contact_gmt_diff!=-1) {
                    contact_gmt_diff = contact_gmt_diff>128 ? 256-contact_gmt_diff : 0-contact_gmt_diff;
                    diff=(int)local_gmt_diff-(int)contact_gmt_diff*60*60/2;
                    final_time = dbei.timestamp - diff;
                }
            }
        }
        event_time = *localtime(&final_time);
    }
    this_templateset = dat->dwFlags & MWF_LOG_RTL ? dat->rtl_templates : dat->ltr_templates;
    
    if(dbei.eventType == EVENTTYPE_STATUSCHANGE)
        szTemplate = this_templateset->szTemplates[TMPL_STATUSCHG];
    else if(dbei.eventType == EVENTTYPE_ERRMSG)
        szTemplate = this_templateset->szTemplates[TMPL_ERRMSG];
    else {
        if(dat->dwFlags & MWF_LOG_GROUPMODE)
            szTemplate = isSent ? (g_groupBreak ? this_templateset->szTemplates[TMPL_GRPSTARTOUT] : this_templateset->szTemplates[TMPL_GRPINNEROUT]) : 
                                  (g_groupBreak ? this_templateset->szTemplates[TMPL_GRPSTARTIN] : this_templateset->szTemplates[TMPL_GRPINNERIN]);
        else
            szTemplate = isSent ? this_templateset->szTemplates[TMPL_MSGOUT] : this_templateset->szTemplates[TMPL_MSGIN];
    }

    iTemplateLen = _tcslen(szTemplate);
    showTime = dat->dwFlags & MWF_LOG_SHOWTIME;
    showDate = dat->dwFlags & MWF_LOG_SHOWDATES;

    while(i < iTemplateLen) {
        ci = szTemplate[i];
        if(ci == '%') {
            cc = szTemplate[i + 1];
            skipToNext = FALSE;
            skipFont = FALSE;
            /*
             * handle modifiers
             */
            while(cc == '#' || cc == '$' || cc == '&') {
                switch (cc) {
                    case '#':
                        if(!dat->isHistory) {
                            skipToNext = TRUE;
                            goto skip;
                        }
                        else {
                            i++;
                            cc = szTemplate[i + 1];
                            continue;
                        }
                    case '$':
                        if(dat->isHistory) {
                            skipToNext = TRUE;
                            goto skip;
                        }
                        else {
                            i++;
                            cc = szTemplate[i + 1];
                            continue;
                        }
                    case '&':
                        i++;
                        cc = szTemplate[i + 1];
                        skipFont = TRUE;
                }
            }
            switch(cc) {
                case 'I':
                {
                    if(dat->dwFlags & MWF_LOG_SHOWICONS) {
                        int icon;
                        if((dat->dwFlags & MWF_LOG_INOUTICONS) && dbei.eventType == EVENTTYPE_MESSAGE)
                            icon = isSent ? LOGICON_OUT : LOGICON_IN;
                        else {
                            switch (dbei.eventType) {
                                case EVENTTYPE_URL:
                                    icon = LOGICON_URL;
                                    break;
                                case EVENTTYPE_FILE:
                                    icon = LOGICON_FILE;
                                    break;
                                case EVENTTYPE_STATUSCHANGE:
                                    icon = LOGICON_STATUS;
                                    break;
                                case EVENTTYPE_ERRMSG:
                                    icon = LOGICON_ERROR;
                                    break;
                                default:
                                    icon = LOGICON_MSG;
                                    break;
                            }
                        }
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s  #~#%01d%c%s ", rtfFonts[MSGDLGFONTCOUNT], icon, isSent ? '>' : '<', rtfFonts[isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset]);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                }
                case 'D':           // long date
                    if(showTime && showDate) {
                        szFinalTimestamp = Template_MakeRelativeDate(dat, final_time, g_groupBreak, 'D');
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", szFinalTimestamp);
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %s", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], szFinalTimestamp);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case 'E':           // short date...
                    if(showTime && showDate) {
                        szFinalTimestamp = Template_MakeRelativeDate(dat, final_time, g_groupBreak, 'E');
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", szFinalTimestamp);
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %s", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], szFinalTimestamp);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case 'a':           // 12 hour
                case 'h':           // 24 hour
                    if(showTime) {
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, cc == 'h' ? "%02d" : "%2d", cc == 'h' ? event_time.tm_hour : (event_time.tm_hour > 12 ? event_time.tm_hour - 12 : event_time.tm_hour));
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, cc == 'h' ? "%s %02d" : "%s %2d", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], cc == 'h' ? event_time.tm_hour : (event_time.tm_hour > 12 ? event_time.tm_hour - 12 : event_time.tm_hour));
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case 'm':           // minute
                    if(showTime) {
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "02d", event_time.tm_min);
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %02d", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], event_time.tm_min);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case 's':           //second
                    if(showTime && dat->dwFlags & MWF_LOG_SHOWSECONDS) {
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%02d", event_time.tm_sec);
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %02d", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], event_time.tm_sec);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case 'p':            // am/pm symbol
                    if(showTime) {
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", event_time.tm_hour > 11 ? "PM" : "AM");
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %s", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], event_time.tm_hour > 11 ? "PM" : "AM");
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case 'o':            // month
                    if(showTime && showDate) {
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%02d", event_time.tm_mon + 1);
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %02d", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], event_time.tm_mon + 1);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case'O':            // month (name)
                    if(showTime && showDate) {
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", months_translated[event_time.tm_mon]);
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %s", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], months_translated[event_time.tm_mon]);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case 'd':           // day of month
                    if(showTime && showDate) {
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%02d", event_time.tm_mday);
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %02d", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], event_time.tm_mday);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case 'w':           // day of week
                    if(showTime && showDate) {
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", weekDays_translated[event_time.tm_wday]);
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %s", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], weekDays_translated[event_time.tm_wday]);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case 'y':           // year
                    if(showTime && showDate) {
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%02d", event_time.tm_year + 1900);
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %02d", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], event_time.tm_year + 1900);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case 'R':
                case 'r':           // long date
                    if(showTime && showDate) {
                        szFinalTimestamp = Template_MakeRelativeDate(dat, final_time, g_groupBreak, cc);
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", szFinalTimestamp);
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %s", rtfFonts[isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset], szFinalTimestamp);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                case 'S':           // symbol
                {
                    if(dat->dwFlags & MWF_LOG_SYMBOLS) {
                        if((dat->dwFlags & MWF_LOG_INOUTICONS) && dbei.eventType == EVENTTYPE_MESSAGE)
                            c = isSent ? 0x37 : 0x38;
                        else {
                            switch(dbei.eventType) {
                                case EVENTTYPE_MESSAGE:
                                    c = 0xaa;
                                    break;
                                case EVENTTYPE_FILE:
                                    c = 0xcd;
                                    break;
                                case EVENTTYPE_URL:
                                    c = 0xfe;
                                    break;
                                case EVENTTYPE_STATUSCHANGE:
                                    c = 0x4e;
                                    break;
                                case EVENTTYPE_ERRMSG:
                                    c = 0x72;;
                             }
                        }
                        if(skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%c%s ", c, rtfFonts[isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset]);
                        else
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %c%s ", isSent ? rtfFonts[MSGFONTID_SYMBOLS_OUT] : rtfFonts[MSGFONTID_SYMBOLS_IN], c, rtfFonts[isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset]);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                }
                case 'n':           // hard line break
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\par");
                    break;
                case 'l':           // soft line break
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\line");
                    break;
                case 'N':           // nickname
                {
                    if(dat->dwFlags & MWF_LOG_SHOWNICK || dbei.eventType == EVENTTYPE_STATUSCHANGE) {
                        szName = isSent ? szMyName : szYourName;
                        if(!skipFont)
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? MSGFONTID_MYNAME + iFontIDOffset : MSGFONTID_YOURNAME + iFontIDOffset]);
                        AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, "%s", szName);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                }
                case 'U':            // UIN
                    if(!skipFont)
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? MSGFONTID_MYNAME + iFontIDOffset : MSGFONTID_YOURNAME + iFontIDOffset]);
                    AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, "%s", isSent ? dat->myUin : dat->uin);
                    break;
                case 'e':           // error message
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %s", rtfFonts[MSGFONTID_ERROR], dbei.szModule);
                    break;
                case 'M':           // message
                {
                    switch (dbei.eventType) {
                        case EVENTTYPE_MESSAGE:
                        case EVENTTYPE_STATUSCHANGE:
                        case EVENTTYPE_ERRMSG:
                        {
                            TCHAR *msg, *formatted;
                            int wlen;
                            if(dbei.eventType == EVENTTYPE_STATUSCHANGE || dbei.eventType == EVENTTYPE_ERRMSG) {
                                if(dbei.eventType == EVENTTYPE_ERRMSG && dbei.cbBlob == 0)
                                    break;
                                if(!skipFont)
                                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[dbei.eventType == EVENTTYPE_STATUSCHANGE ? H_MSGFONTID_STATUSCHANGES : MSGFONTID_MYMSG]);
                            }
                            else {
                                if(!skipFont)
                                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset]);
                            }
                #if defined( _UNICODE )
                            {
                                int msglen = lstrlenA((char *) dbei.pBlob) + 1;

                                if(dbei.eventType == EVENTTYPE_MESSAGE && !isSent)
                                    dat->stats.lastReceivedChars = msglen - 1;
                                if ((dbei.cbBlob >= (DWORD)(2 * msglen)) && !(dat->sendMode & SMODE_FORCEANSI)) {
                                    msg = (wchar_t *) &dbei.pBlob[msglen];
                                    wlen = safe_wcslen(msg, (dbei.cbBlob - msglen) / 2);
                                    if(wlen <= (msglen - 1) && wlen > 0){
                                        TrimMessage(msg);
                                        formatted = FormatRaw(dat->dwFlags, msg, MAKELONG(myGlobals.m_FormatWholeWordsOnly, dat->dwEventIsShown & MWF_SHOW_BBCODE));
                                        AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, formatted, MAKELONG(isSent, dat->isHistory));
                                    }
                                    else
                                        goto nounicode;
                                }
                                else {
                nounicode:
                                    msg = (TCHAR *) alloca(sizeof(TCHAR) * msglen);
                                    MultiByteToWideChar(dat->codePage, 0, (char *) dbei.pBlob, -1, msg, msglen);
                                    TrimMessage(msg);
                                    formatted = FormatRaw(dat->dwFlags, msg, MAKELONG(myGlobals.m_FormatWholeWordsOnly, dat->dwEventIsShown & MWF_SHOW_BBCODE));
                                    AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, formatted, MAKELONG(isSent, dat->isHistory));
                                }
                            }
                #else   // unicode
                            msg = (char *) dbei.pBlob;
                            if(dbei.eventType == EVENTTYPE_MESSAGE && !isSent)
                                dat->stats.lastReceivedChars = lstrlenA(msg);
                            TrimMessage(msg);
                            formatted = FormatRaw(dat->dwFlags, msg, MAKELONG(myGlobals.m_FormatWholeWordsOnly, dat->dwEventIsShown & MWF_SHOW_BBCODE));
                            AppendToBufferWithRTF(MAKELONG(isSent, dat->isHistory), &buffer, &bufferEnd, &bufferAlloced, "%s", formatted);
                #endif      // unicode
                            break;
                        }
                        case EVENTTYPE_URL:
                            if(!skipFont)
                                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? MSGFONTID_MYMISC + iFontIDOffset : MSGFONTID_YOURMISC + iFontIDOffset]);
                            AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, "%s", dbei.pBlob);
                            if ((dbei.pBlob + lstrlenA(dbei.pBlob) + 1) != NULL && lstrlenA(dbei.pBlob + lstrlenA(dbei.pBlob) + 1))
                                AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, " (%s)", dbei.pBlob + lstrlenA(dbei.pBlob) + 1);
                            break;
                        case EVENTTYPE_FILE:
                            if(!skipFont)
                                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? MSGFONTID_MYMISC + iFontIDOffset : MSGFONTID_YOURMISC + iFontIDOffset]);
                            if ((dbei.pBlob + sizeof(DWORD) + lstrlenA(dbei.pBlob + sizeof(DWORD)) + 1) != NULL && lstrlenA(dbei.pBlob + sizeof(DWORD) + lstrlenA(dbei.pBlob + sizeof(DWORD)) + 1))
                                AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, "%s (%s)", dbei.pBlob + sizeof(DWORD), dbei.pBlob + sizeof(DWORD) + lstrlenA(dbei.pBlob + sizeof(DWORD)) + 1);
                            else
                                AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, "%s", dbei.pBlob + sizeof(DWORD));
                            break;
                    }
                    break;
                }
                case '*':       // bold
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, isBold ? "\\b0 " : "\\b ");
                    isBold = !isBold;
                    break;
                case '/':       // italic
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, isItalic ? "\\i0 " : "\\i ");
                    isItalic = !isItalic;
                    break;
                case '_':       // italic
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, isUnderline ? "\\ul0 " : "\\ul ");
                    isUnderline = !isUnderline;
                    break;
                case '-':       // grid line
                {
                    TCHAR color = szTemplate[i + 2];
                    if(color >= '0' && color <= '4') {
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\par%s\\sl-1", rtfFonts[MSGDLGFONTCOUNT]);
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szSep1, MSGDLGFONTCOUNT + 5 + (color - '0'));
                        i++;
                    }
                    else {
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\par%s\\sl-1", rtfFonts[MSGDLGFONTCOUNT]);
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szSep1, MSGDLGFONTCOUNT + 4);
                    }
                    break;
                }
                case '~':       // font break (switch to default font...)
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, rtfFonts[isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset]);
                    break;
                case 'H':           // highlight
                {
                    TCHAR color = szTemplate[i + 2];
                    if(color >= '0' && color <= '4') {
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight%d", MSGDLGFONTCOUNT + 5 + (color - '0'));
                        i++;
                    }
                    else
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight%d", (dat->dwFlags & MWF_LOG_INDIVIDUALBKG) ? (MSGDLGFONTCOUNT + 1 + (isSent ? 1 : 0)) : MSGDLGFONTCOUNT + 3);
                    break;
                }
                case '|':       // tab
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\tab");
                    break;
                case 'f':       // font tag...
                {
                    TCHAR code = szTemplate[i + 2];
                    int fontindex = -1;
                    switch(code) {
                        case 'd':
                            fontindex = isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset;
                            break;
                        case 'n':
                            fontindex = isSent ? MSGFONTID_MYNAME + iFontIDOffset : MSGFONTID_YOURNAME + iFontIDOffset;
                            break;
                        case 'm':
                            fontindex = isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset;
                            break;
                        case 'M':
                            fontindex = isSent ? MSGFONTID_MYMISC + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset;
                            break;
                        case 's':
                            fontindex = isSent ? MSGFONTID_SYMBOLS_OUT : MSGFONTID_SYMBOLS_IN;
                            break;
                    }
                    if(fontindex != -1) {
                        i++;
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[fontindex]);
                    }
                    else
                        skipToNext = TRUE;
                    break;
                }
                case 'c':       // font color (using one of the predefined 5 colors)
                {
                    TCHAR color = szTemplate[i + 2];
                    if(color >= '0' && color <= '4') {
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\cf%d ", MSGDLGFONTCOUNT + 5 + (color - '0'));
                        i++;
                    }
                    else
                        skipToNext = TRUE;
                    break;
                }
            }
skip:            
            if(skipToNext) {
                i++;
                while(szTemplate[i] != '%' && i < iTemplateLen) i++;
            }
            else
                i += 2;
        }
        else {
#if defined(_UNICODE)
            char temp[24];
            mir_snprintf(temp, 24, "{\\uc1\\u%d?}", (int)ci);
            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, temp);
#else
            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%c", ci);
#endif            
            i++;
        }
    }
    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szMicroLf);
    
    if(streamData->dbei == 0)
        free(dbei.pBlob);
    
    dat->iLastEventType = MAKELONG(dbei.flags, dbei.eventType);
    dat->lastEventTime = dbei.timestamp;
    return buffer;
}
    
static DWORD CALLBACK LogStreamInEvents(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
    struct LogStreamData *dat = (struct LogStreamData *) dwCookie;
    
    if (dat->buffer == NULL) {
        dat->bufferOffset = 0;
        switch (dat->stage) {
            case STREAMSTAGE_HEADER:
                dat->buffer = CreateRTFHeader(dat->dlgDat);
                dat->stage = STREAMSTAGE_EVENTS;
                break;
            case STREAMSTAGE_EVENTS:
                if (dat->eventsToInsert) {
                    do {
                        dat->buffer = Template_CreateRTFFromDbEvent(dat->dlgDat, dat->hContact, dat->hDbEvent, !dat->isEmpty, dat);
                        if (dat->buffer)
                            dat->hDbEventLast = dat->hDbEvent;
                        dat->hDbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, (WPARAM) dat->hDbEvent, 0);
                        if (--dat->eventsToInsert == 0)
                            break;
                    } while (dat->buffer == NULL && dat->hDbEvent);
                    if (dat->buffer) {
                        dat->isEmpty = 0;
                        break;
                    }
                }
                dat->stage = STREAMSTAGE_TAIL;
                //fall through
            case STREAMSTAGE_TAIL:{
                dat->buffer = CreateRTFTail(dat->dlgDat);
                dat->stage = STREAMSTAGE_STOP;
                break;
            }
            case STREAMSTAGE_STOP:
                *pcb = 0;
                return 0;
        }
        dat->bufferLen = lstrlenA(dat->buffer);
    }
    *pcb = min(cb, dat->bufferLen - dat->bufferOffset);
    CopyMemory(pbBuff, dat->buffer + dat->bufferOffset, *pcb);
    dat->bufferOffset += *pcb;
    if (dat->bufferOffset == dat->bufferLen) {
        free(dat->buffer);
        dat->buffer = NULL;
    }
    return 0;
}

void StreamInEvents(HWND hwndDlg, HANDLE hDbEventFirst, int count, int fAppend, DBEVENTINFO *dbei_s)
{
    EDITSTREAM stream = { 0 };
    struct LogStreamData streamData = { 0 };
    struct MessageWindowData *dat = (struct MessageWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
    CHARRANGE oldSel, sel;
    CONTACTINFO ci;
    HWND hwndrtf;
    LONG startAt = 0;
    FINDTEXTEXA fi;
    
    // IEVIew MOD Begin
    if (dat->hwndLog != 0) {
        IEVIEWEVENT event;
        event.cbSize = sizeof(IEVIEWEVENT);
        event.hwnd = dat->hwndLog;
        event.hContact = dat->hContact;
#if defined(_UNICODE)
        event.dwFlags = (dat->dwFlags & MWF_LOG_RTL) ? IEEF_RTL : 0;
        if(dat->sendMode & SMODE_FORCEANSI) {
            event.dwFlags |= IEEF_NO_UNICODE;
            event.codepage = dat->codePage;
        }
        else
            event.codepage = 0;
#else
        event.dwFlags = ((dat->dwFlags & MWF_LOG_RTL) ? IEEF_RTL : 0) | IEEF_NO_UNICODE;
        event.codepage = 0;
#endif        
        if (!fAppend) {
            event.iType = IEE_CLEAR_LOG;
            CallService(MS_IEVIEW_EVENT, 0, (LPARAM)&event);
        }
        event.iType = IEE_LOG_EVENTS;
        event.hDbEventFirst = hDbEventFirst;
        event.count = count;
        CallService(MS_IEVIEW_EVENT, 0, (LPARAM)&event);
        return;
    }
    // IEVIew MOD End

    // separator strings used for grid lines, message separation and so on...
    
    dwExtraLf = myGlobals.m_ExtraMicroLF;

    strcpy(szSep0, fAppend ? "\\par%s\\sl-1" : "%s\\sl-1");
    
    mir_snprintf(szSep1, 151, "\\highlight%s \\par\\sl0%s", "%d", rtfFonts[H_MSGFONTID_YOURTIME]);
    strcpy(szSep2, fAppend ? "\\par\\sl0" : "\\sl1000");
    mir_snprintf(szMicroLf, sizeof(szMicroLf), "%s\\par\\sl-1%s", rtfFonts[MSGDLGFONTCOUNT], rtfFonts[MSGDLGFONTCOUNT]);
    mir_snprintf(szExtraLf, sizeof(szExtraLf), dat->dwFlags & MWF_LOG_INDIVIDUALBKG ? "\\par\\sl-%d\\highlight%s \\par" : "\\par\\sl-%d \\par", dwExtraLf * 15, "%d");
              
    strcpy(szMsgPrefixColon, ": ");
    strcpy(szMsgPrefixNoColon, " ");

    ZeroMemory(&ci, sizeof(ci));
    ci.cbSize = sizeof(ci);
    ci.hContact = NULL;
    ci.szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
    ci.dwFlag = CNF_DISPLAY;
    if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
        // CNF_DISPLAY always returns a string type
        szMyName = ci.pszVal;
    }
    else
        szMyName = NULL;
    szYourName = (char *) dat->szNickname;
    
    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_HIDESELECTION, TRUE, 0);
    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXGETSEL, 0, (LPARAM) & oldSel);
    streamData.hContact = dat->hContact;
    streamData.hDbEvent = hDbEventFirst;
    streamData.dlgDat = dat;
    streamData.eventsToInsert = count;
    streamData.isEmpty = fAppend ? GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LOG)) == 0 : 1;
    streamData.dbei = dbei_s;
    stream.pfnCallback = LogStreamInEvents;
    stream.dwCookie = (DWORD_PTR) & streamData;
    
    if (fAppend) {
        GETTEXTLENGTHEX gtxl = {0};
#if defined(_UNICODE)
        gtxl.codepage = 1200;
        gtxl.flags = GTL_DEFAULT | GTL_PRECISE | GTL_NUMCHARS;
#else
        gtxl.codepage = CP_ACP;
        gtxl.flags = GTL_DEFAULT | GTL_PRECISE;
#endif        
        sel.cpMin = sel.cpMax = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LOG));
        SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXSETSEL, 0, (LPARAM) & sel);

        fi.chrg.cpMin = SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETTEXTLENGTHEX, (WPARAM)&gtxl, 0);
    }
    else
        fi.chrg.cpMin = 0;

    startAt = fi.chrg.cpMin;
    
    hwndrtf = GetDlgItem(hwndDlg, IDC_LOG);

    SendMessage(hwndrtf, WM_SETREDRAW, FALSE, 0);

    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_STREAMIN, fAppend ? SFF_SELECTION | SF_RTF : SF_RTF, (LPARAM) & stream);
    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXSETSEL, 0, (LPARAM) & oldSel);
    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_HIDESELECTION, FALSE, 0);
    dat->hDbEventLast = streamData.hDbEventLast;
    
    if (fAppend) {
        GETTEXTLENGTHEX gtxl = {0};
#if defined(_UNICODE)
        gtxl.codepage = 1200;
        gtxl.flags = GTL_DEFAULT | GTL_PRECISE | GTL_NUMCHARS;
#else
        gtxl.codepage = CP_ACP;
        gtxl.flags = GTL_DEFAULT | GTL_PRECISE;
#endif        
        sel.cpMax = SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETTEXTLENGTHEX, (WPARAM)&gtxl, 0);
        sel.cpMin = sel.cpMax - 1;
        SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXSETSEL, 0, (LPARAM) & sel);
        SendDlgItemMessage(hwndDlg, IDC_LOG, EM_REPLACESEL, FALSE, (LPARAM)_T(""));
    }
    ReplaceIcons(hwndDlg, dat, startAt, fAppend);

    if(ci.pszVal)
        miranda_sys_free(ci.pszVal);
}

void ReplaceIcons(HWND hwndDlg, struct MessageWindowData *dat, LONG startAt, int fAppend)
{
    FINDTEXTEXA fi;
    CHARFORMAT2 cf2;
    HWND hwndrtf;
    IRichEditOle *ole;
    TEXTRANGEA tr;
    COLORREF crDefault = DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR);

    struct MsgLogIcon theIcon;
    char trbuffer[20];
    tr.lpstrText = trbuffer;
    
    hwndrtf = GetDlgItem(hwndDlg, IDC_LOG);
    fi.chrg.cpMin = startAt;

    if(dat->dwFlags & MWF_LOG_SHOWICONS) {
        BYTE bIconIndex = 0;
        CHARRANGE cr;
        fi.lpstrText = "#~#";
        fi.chrg.cpMax = -1;
        ZeroMemory((void *)&cf2, sizeof(cf2));
        cf2.cbSize = sizeof(cf2);
        cf2.dwMask = CFM_BACKCOLOR;

        SendMessage(hwndrtf, EM_GETOLEINTERFACE, 0, (LPARAM)&ole);
        while (SendMessageA(hwndrtf, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&fi) > -1) {
            cr.cpMin = fi.chrgText.cpMin;
            cr.cpMax = fi.chrgText.cpMax + 2;
            SendMessage(hwndrtf, EM_EXSETSEL, 0, (LPARAM)&cr);
            
            tr.chrg.cpMin = fi.chrgText.cpMin + 3;
            tr.chrg.cpMax = fi.chrgText.cpMin + 5;
            SendMessageA(hwndrtf, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
            if((BYTE)(trbuffer[0] - '0') >= NR_LOGICONS ) {
                fi.chrg.cpMin = fi.chrgText.cpMax + 6;
                continue;
            }
#if defined _CACHED_ICONS
            bIconIndex = ((BYTE)trbuffer[0] - (BYTE)'0') * 3;

            if(dat->dwFlags & MWF_LOG_INDIVIDUALBKG) {
                if(trbuffer[1] == '<')
                    bIconIndex += 1;
                else
                    bIconIndex += 2;
            }
            ImageDataInsertBitmap(ole, msgLogIcons[bIconIndex].hBmp);
#else            
            bIconIndex = ((BYTE)trbuffer[0] - (BYTE)'0');
            SendMessage(hwndrtf, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
            CacheIconToBMP(&theIcon, Logicons[bIconIndex], cf2.crBackColor == 0 ? crDefault : cf2.crBackColor, 0, 0);
            ImageDataInsertBitmap(ole, theIcon.hBmp);
            DeleteCachedIcon(&theIcon);
#endif
            fi.chrg.cpMin = cr.cpMax + 6;
        }
        ReleaseRichEditOle(ole);
    }

    /*
     * do smiley replacing, using the service
     */

    if(myGlobals.g_SmileyAddAvail && myGlobals.m_SmileyPluginEnabled) {
        CHARRANGE sel;
        SMADD_RICHEDIT2 smadd;

        sel.cpMin = startAt;
        sel.cpMax = -1;

        smadd.cbSize = sizeof(smadd);
        smadd.hwndRichEditControl = GetDlgItem(hwndDlg, IDC_LOG);
        smadd.Protocolname = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
        if(startAt > 0)
            smadd.rangeToReplace = &sel;
        else
            smadd.rangeToReplace = NULL;
        smadd.disableRedraw = TRUE;
        if(dat->doSmileys)
            CallService(MS_SMILEYADD_REPLACESMILEYS, TABSRMM_SMILEYADD_BKGCOLORMODE, (LPARAM)&smadd);
    }
    
// do formula-replacing    
#ifdef __MATHMOD_SUPPORT    
	// mathMod begin
	if (myGlobals.m_MathModAvail)
	{
			 TMathRicheditInfo mathReplaceInfo;
			 CHARRANGE mathNewSel;
			 mathNewSel.cpMin=startAt;
			 mathNewSel.cpMax=-1;
			 mathReplaceInfo.hwndRichEditControl = GetDlgItem(hwndDlg, IDC_LOG);
			 if (startAt > 0) mathReplaceInfo.sel = & mathNewSel; else mathReplaceInfo.sel=0;
			 mathReplaceInfo.disableredraw = TRUE;
			 CallService(MATH_RTF_REPLACE_FORMULAE,0, (LPARAM)&mathReplaceInfo);
	}
	// mathMod end
#endif    

    SendMessage(hwndDlg, DM_FORCESCROLL, 0, 0);
    SendDlgItemMessage(hwndDlg, IDC_LOG, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(GetDlgItem(hwndDlg, IDC_LOG), NULL, FALSE);
    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_QUOTE), dat->hDbEventLast != NULL);
}

/* 
 * NLS functions (for unicode version only) encoding stuff..
 */

static BOOL CALLBACK LangAddCallback(LPCSTR str)
{
	int i, count;
	UINT cp;

    cp = atoi(str);
	count = sizeof(cpTable)/sizeof(cpTable[0]);
	for (i=0; i<count && cpTable[i].cpId!=cp; i++);
	if (i < count) {
        AppendMenuA(myGlobals.g_hMenuEncoding, MF_STRING, cp, Translate(cpTable[i].cpName));
	}
	return TRUE;
}

void BuildCodePageList()
{
    myGlobals.g_hMenuEncoding = CreateMenu();
    AppendMenuA(myGlobals.g_hMenuEncoding, MF_STRING, 500, Translate("Use default codepage"));
    AppendMenuA(myGlobals.g_hMenuEncoding, MF_SEPARATOR, 0, 0);
    EnumSystemCodePagesA(LangAddCallback, CP_INSTALLED);
}

static char *Template_MakeRelativeDate(struct MessageWindowData *dat, time_t check, int groupBreak, TCHAR code)
{
    static char szResult[100];
    DBTIMETOSTRING dbtts;
    struct tm tm_now, tm_today;
    
    time_t now = time(NULL);
    time_t today;
    
    dbtts.cbDest = 70;;
    dbtts.szDest = szResult;

    szResult[0] = 0;
    
    tm_now = *localtime(&now);
    tm_today = tm_now;
    tm_today.tm_hour = tm_today.tm_min = tm_today.tm_sec = 0;
    today = mktime(&tm_today);

    if((code == 'R' || code == 'r') && check >= today) {
        strcpy(szResult, szToday);
    }
    else if((code == 'R' || code == 'r') && check > (today - 86400)) {
        strcpy(szResult, szYesterday);
    }
    else {
        if(code == 'D' || code == 'R')
            dbtts.szFormat = "D";
        else if(code == 'T')
            dbtts.szFormat = "s";
        else if(code == 't')
            dbtts.szFormat = "t";
        else
            dbtts.szFormat = "d";
        CallService(MS_DB_TIME_TIMESTAMPTOSTRING, check, (LPARAM) & dbtts);
    }
    return szResult;
}   

/*
 * decodes UTF-8 to unicode
 * taken from jabber protocol implementation and slightly modified
 * free() the return value
 */

#if defined(_UNICODE)

WCHAR *Utf8_Decode(const char *str)
{
	int i, len;
	char *p;
	WCHAR *wszTemp = NULL;

	if (str == NULL) return NULL;

	len = strlen(str);

    if ((wszTemp = (WCHAR *) malloc(sizeof(TCHAR) * (len + 2))) == NULL)
		return NULL;
	p = (char *) str;
	i = 0;
	while (*p) {
		if ((*p & 0x80) == 0)
			wszTemp[i++] = *(p++);
		else if ((*p & 0xe0) == 0xe0) {
			wszTemp[i] = (*(p++) & 0x1f) << 12;
			wszTemp[i] |= (*(p++) & 0x3f) << 6;
			wszTemp[i++] |= (*(p++) & 0x3f);
		}
		else {
			wszTemp[i] = (*(p++) & 0x3f) << 6;
			wszTemp[i++] |= (*(p++) & 0x3f);
		}
	}
	wszTemp[i] = (TCHAR)'\0';
	return wszTemp;
}

/*
 * convert unicode to UTF-8
 * code taken from jabber protocol implementation and slightly modified.
 * free() the return value
 */

char *Utf8_Encode(const WCHAR *str)
{
	unsigned char *szOut = NULL;
	int len, i;
	const WCHAR *wszTemp, *w;
    
	if (str == NULL) 
        return NULL;

    wszTemp = str;

	// Convert unicode to utf8
	len = 0;
	for (w=wszTemp; *w; w++) {
		if (*w < 0x0080) len++;
		else if (*w < 0x0800) len += 2;
		else len += 3;
	}

	if ((szOut = (unsigned char *) malloc(len + 2)) == NULL)
		return NULL;

	i = 0;
	for (w=wszTemp; *w; w++) {
		if (*w < 0x0080)
			szOut[i++] = (unsigned char) *w;
		else if (*w < 0x0800) {
			szOut[i++] = 0xc0 | ((*w) >> 6);
			szOut[i++] = 0x80 | ((*w) & 0x3f);
		}
		else {
			szOut[i++] = 0xe0 | ((*w) >> 12);
			szOut[i++] = 0x80 | (((*w) >> 6) & 0x3f);
			szOut[i++] = 0x80 | ((*w) & 0x3f);
		}
	}
	szOut[i] = '\0';
	return (char *) szOut;
}

#endif
