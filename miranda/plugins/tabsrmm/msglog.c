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

#ifdef __MATHMOD_SUPPORT
//mathMod begin
#define MTH_GET_RTF_BITMAPTEXT "Math/GetRTFBitmapText"
#define MTH_FREE_RTF_BITMAPTEXT "Math/FreeRTFBitmapText"
//mathMod end
#endif

int _log(const char *fmt, ...);
static char *MakeRelativeDate(struct MessageWindowData *dat, time_t check, int groupBreak);
void ReplaceIcons(HWND hwndDlg, struct MessageWindowData *dat, LONG startAt, int fAppend);
extern int g_SmileyAddAvail;

#if defined(_STREAMTHREADING)
    extern int g_StreamThreadRunning;
    extern struct StreamJob StreamJobs[];
    extern int volatile g_StreamJobCurrent;
    extern CRITICAL_SECTION sjcs;
    extern HANDLE g_hStreamThread;
#endif
// ole Icon stuff...

char szSep0[40], szSep1[152], szSep2[40];
int g_groupBreak = TRUE;
static char *szMyName, *szYourName;
static char *szDivider = "\\strike-----------------------------------------------------------------------------------------------------------------------------------\\strike0";
static char *szGroupedSeparator = "> ";

extern void ImageDataInsertBitmap(IRichEditOle *ole, HBITMAP hBm);
extern int CacheIconToBMP(struct MsgLogIcon *theIcon, HICON hIcon, COLORREF backgroundColor, int sizeX, int sizeY);
extern void DeleteCachedIcon(struct MsgLogIcon *theIcon);

extern void ReleaseRichEditOle(IRichEditOle *ole);

extern HICON g_iconIn, g_iconOut, g_iconErr;

#if defined(_STREAMTHREADING)
    extern int g_StreamThreadRunning;
#endif

static int logPixelSY;

static char szToday[22], szYesterday[22];

#define LOGICON_MSG  0
#define LOGICON_URL  1
#define LOGICON_FILE 2
#define LOGICON_OUT 3
#define LOGICON_IN 4
#define LOGICON_STATUS 5
#define LOGICON_ERROR 6

struct MsgLogIcon msgLogIcons[NR_LOGICONS * 3];

static PBYTE pLogIconBmpBits[3];
static int logIconBmpSize[sizeof(pLogIconBmpBits) / sizeof(pLogIconBmpBits[0])];

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

extern HMENU g_hMenuEncoding;

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
    if(strlen(szToday) > 0)
        strncat(szToday, ", ", 20);
    strncpy(szYesterday, Translate("Yesterday"), 20);
    if(strlen(szYesterday) > 0)
        strncat(szYesterday, ", ", 20);
}

void UncacheMsgLogIcons()
{
    int i;

    for(i = 0; i < 3 * NR_LOGICONS; i++)
        DeleteCachedIcon(&msgLogIcons[i]);
}

/*
 * cache copies of all our msg log icons with 3 background colors to speed up the
 * process of inserting icons into the RichEdit control.
 */

void CacheMsgLogIcons()
{
    HICON icons[NR_LOGICONS];
    int iCounter = 0;
    int i;
    int size;
    
    icons[0] = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
    icons[1] = LoadSkinnedIcon(SKINICON_EVENT_URL);
    icons[2] = LoadSkinnedIcon(SKINICON_EVENT_FILE);
    icons[3] = g_iconOut;
    icons[4] = g_iconIn;
    icons[5] = LoadSkinnedIcon(SKINICON_OTHER_USERONLINE);
    icons[6] = g_iconErr;
    
    for(i = 0; i < NR_LOGICONS; i++) {
        if(icons[i] == g_iconOut || icons[i] == g_iconIn)
            size = 0;
        else
            size = 16;          // force mirandas icons into small mode (16x16 pixels - on some systems, they load with incorrect size..?)
        CacheIconToBMP(&msgLogIcons[iCounter++], icons[i], DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR), size, size);
        CacheIconToBMP(&msgLogIcons[iCounter++], icons[i], DBGetContactSettingDword(NULL, SRMSGMOD_T, "inbg", RGB(255,255,255)), size, size);
        CacheIconToBMP(&msgLogIcons[iCounter++], icons[i], DBGetContactSettingDword(NULL, SRMSGMOD_T, "outbg", RGB(255,255,255)), size, size);
    }
}
static void AppendToBuffer(char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
{
    va_list va;
    int charsDone;

    va_start(va, fmt);
    for (;;) {
        charsDone = _vsnprintf(*buffer + *cbBufferEnd, *cbBufferAlloced - *cbBufferEnd, fmt, va);
        if (charsDone >= 0)
            break;
        *cbBufferAlloced += 1024;
        *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
    }
    va_end(va);
    *cbBufferEnd += charsDone;
}

#if defined( _UNICODE )

static int AppendUnicodeToBuffer(char **buffer, int *cbBufferEnd, int *cbBufferAlloced, TCHAR * line)
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
        if (*line == '\r' && line[1] == '\n') {
// XXX indent mod
			/*
			CopyMemory(d, "\\par ", 5);
            line++;
            d += 5; */
			CopyMemory(d,"\\line ",6);
			line++;
			d += 6;
// XXX indent mod
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
static int AppendToBufferWithRTF(char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
{
    va_list va;
    int charsDone, i;

    va_start(va, fmt);
    for (;;) {
        charsDone = _vsnprintf(*buffer + *cbBufferEnd, *cbBufferAlloced - *cbBufferEnd, fmt, va);
        if (charsDone >= 0)
            break;
        *cbBufferAlloced += 1024;
        *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
    }
    va_end(va);
    *cbBufferEnd += charsDone;
    for (i = *cbBufferEnd - charsDone; (*buffer)[i]; i++) {
        if ((*buffer)[i] == '\r' && (*buffer)[i + 1] == '\n') {
            if (*cbBufferEnd + 5 > *cbBufferAlloced) {
                *cbBufferAlloced += 1024;
                *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
            }
// XXX mod indent            
			/*
			MoveMemory(*buffer + i + 5, *buffer + i + 2, *cbBufferEnd - i - 1);
            CopyMemory(*buffer + i, "\\par ", 5);
            *cbBufferEnd += 3;
			*/

            MoveMemory(*buffer + i + 6, *buffer +i + 2, *cbBufferEnd - i - 1);
            CopyMemory(*buffer+i,"\\line ",6);
            *cbBufferEnd+=4;
// XXX end mod

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

    // XXX rtl AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "{\\rtf1\\ansi\\deff0{\\fonttbl");
	if (dat->dwFlags & MWF_LOG_RTL) 
		AppendToBuffer(&buffer,&bufferEnd,&bufferAlloced,"{\\rtf1\\ansi\\deff0\\rtldoc{\\fonttbl");
	else 
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "{\\rtf1\\ansi\\deff0{\\fonttbl");

	// xxx rtl

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

    // xxx RTL-Support
	if (dat->dwFlags & MWF_LOG_RTL) 
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "}\\rtlpar");
	else 
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "}\\pard");

// XXX mod: indent
	if(dat->dwFlags & MWF_LOG_INDENT) {
		int iIndent = (int) DBGetContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", 0) * 15;
        int rIndent = (int) DBGetContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", 0) * 15;
        
		if(iIndent) {
            if(dat->dwFlags & MWF_LOG_RTL)
                AppendToBuffer(&buffer,&bufferEnd,&bufferAlloced,"\\ri%u\\fi-%u\\li%u", iIndent, iIndent, rIndent);
            else
                AppendToBuffer(&buffer,&bufferEnd,&bufferAlloced,"\\li%u\\fi-%u\\ri%u", iIndent, iIndent, rIndent);
        }
	}
    else {
        AppendToBuffer(&buffer,&bufferEnd,&bufferAlloced,"\\li%u\\ri%u\\fi-%u", 3*15, 3*15, 3*15);
    }
// XXX mod end

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
            break;
        case EVENTTYPE_URL:
            if(dat->dwEventIsShown & MWF_SHOW_URLEVENTS)
                return 1;
            break;
        case EVENTTYPE_FILE:
            if(dat->dwEventIsShown & MWF_SHOW_FILEEVENTS)
                return 1;
            break;
    }
    return 0;
}

//free() the return value
static char *CreateRTFFromDbEvent(struct MessageWindowData *dat, HANDLE hContact, HANDLE hDbEvent, int prefixParaBreak, struct LogStreamData *streamData)
{
    char *buffer;
    int bufferAlloced, bufferEnd;
    DBEVENTINFO dbei = { 0 };
    int showColon = 0;
    int isSent = 0;
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
    
// BEGIN MOD#32: Use different fonts for old history events
	// Not working for outgoing messages in History
	// dat->isHistory=(dbei.flags&DBEF_READ) ? TRUE : FALSE;
	if(dat->isHistoryCount>0) {
		dat->isHistoryCount--;
		dat->isHistory=TRUE;
	}
	else dat->isHistory=FALSE;
// END MOD#32

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
            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\line\\highlight%d", MSGDLGFONTCOUNT + 1 + ((LOWORD(dat->iLastEventType) & DBEF_SENT) ? 1 : 0));
        else
            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\line");
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", rtfFonts[H_MSGFONTID_DIVIDERS]);
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
            // separators after message simulating the "grid". draw an empty line (one space) using a 1 pixel font and minimum linespacing.
            // this uses the default background color.
            if(dat->dwFlags & MWF_LOG_GRID) {
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szSep0, rtfFonts[MSGDLGFONTCOUNT]);
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szSep1, MSGDLGFONTCOUNT + 3);
            }
            else
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szSep2);
        }
    }
    /* OnO: highlight start */
    if(dat->dwFlags & MWF_LOG_INDIVIDUALBKG)
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight%d", MSGDLGFONTCOUNT + 1 + ((isSent) ? 1 : 0));

    if ((dat->dwFlags & MWF_LOG_SHOWICONS) && g_groupBreak) {
        int i;
        if((dat->dwEventIsShown & MWF_SHOW_INOUTICONS) && dbei.eventType == EVENTTYPE_MESSAGE) {
            if(isSent)
                i= LOGICON_OUT;
            else
                i= LOGICON_IN;
        }
        else {
            switch (dbei.eventType) {
                case EVENTTYPE_URL:
                    i = LOGICON_URL;
                    break;
                case EVENTTYPE_FILE:
                    i = LOGICON_FILE;
                    break;
                case EVENTTYPE_STATUSCHANGE:
                    i = LOGICON_STATUS;
                    break;
                case EVENTTYPE_ERRMSG:
                    i = LOGICON_ERROR;
                    break;
                default:
                    i = LOGICON_MSG;
                    break;
            }
        }
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s   #~#%01d%c%s  ", rtfFonts[MSGDLGFONTCOUNT], i, isSent ? '>' : '<', rtfFonts[H_MSGFONTID_DIVIDERS]);
    }
    else
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s  ", rtfFonts[H_MSGFONTID_DIVIDERS]);
    
// XXX underline
	if(dat->dwFlags & MWF_LOG_UNDERLINE && dbei.eventType != EVENTTYPE_STATUSCHANGE && dbei.eventType != EVENTTYPE_ERRMSG)
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\ul");
// XXX end mod
    
// XXX rewritten for swap timestamp/nickname
	if (dat->dwFlags & MWF_LOG_SHOWTIME || dat->dwFlags & MWF_LOG_SHOWNICK || !g_groupBreak) {
		char *szName, *szFinalTimestamp, szDummy = '\0';
		BYTE bHideNick = g_groupBreak ? !(dat->dwFlags & MWF_LOG_SHOWNICK) : TRUE;
        DWORD final_time = dbei.timestamp;
        
		if (dat->dwFlags & MWF_LOG_SHOWTIME && (g_groupBreak || dat->dwEventIsShown & MWF_SHOW_MARKFOLLOWUPTS)) {
            showColon = 1;
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
						//dbei.timestamp-=diff;
					}
				}
			}
            
            szFinalTimestamp = MakeRelativeDate(dat, final_time, g_groupBreak);
		}
        else if (!g_groupBreak)
            szFinalTimestamp = szGroupedSeparator;
        else
            szFinalTimestamp = &szDummy;

        if (isSent)
            szName = szMyName;
        else
            szName = szYourName;
        
        if(!bHideNick)
            showColon = 1;

        if(dbei.eventType == EVENTTYPE_STATUSCHANGE || dbei.eventType == EVENTTYPE_ERRMSG) {
            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[H_MSGFONTID_MYTIME]);
            AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s ", szFinalTimestamp);
            if(dbei.eventType == EVENTTYPE_STATUSCHANGE) {
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[H_MSGFONTID_MYNAME]);
                AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s", szName);
            }
            showColon = 0;
            if(dbei.eventType == EVENTTYPE_ERRMSG) {
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[H_MSGFONTID_STATUSCHANGES]);
                AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "\r\n%s", dbei.szModule);
                if(dbei.cbBlob != 0) {
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\r\n%s\\line", rtfFonts[H_MSGFONTID_DIVIDERS]);
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szDivider);
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\line");
                }
            }
        }
        else {
            if((dat->dwFlags & MWF_LOG_SHOWTIME) && !bHideNick) {		// show both...
                if(dat->dwFlags & MWF_LOG_SWAPNICK) {		// first nick, then time..
                    if (dat->isHistory)
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? H_MSGFONTID_MYNAME : H_MSGFONTID_YOURNAME]);
                    else
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? MSGFONTID_MYNAME : MSGFONTID_YOURNAME]);
                    AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s", szName);
                    if (dat->isHistory)
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? H_MSGFONTID_MYTIME : H_MSGFONTID_YOURTIME]);
                    else
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? MSGFONTID_MYTIME : MSGFONTID_YOURTIME]);
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", szFinalTimestamp);
                } else {
                    if (dat->isHistory)
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? H_MSGFONTID_MYTIME : H_MSGFONTID_YOURTIME]);
                    else
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? MSGFONTID_MYTIME : MSGFONTID_YOURTIME]);
                    AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s", szFinalTimestamp);
                    if (dat->isHistory)
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? H_MSGFONTID_MYNAME : H_MSGFONTID_YOURNAME]);
                    else
                        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? MSGFONTID_MYNAME : MSGFONTID_YOURNAME]);
                    /*
                    if(!(dbei.flags & DBEF_SENT) && !strcmp((char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)dat->hContact, 0), "Engelchen"))
                        AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, " (L)  %s  (L) ", szName);
                    else */
                        AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s", szName);
                }
            } else if(dat->dwFlags & MWF_LOG_SHOWTIME || !g_groupBreak) {
                if (dat->isHistory)
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? H_MSGFONTID_MYTIME : H_MSGFONTID_YOURTIME]);
                else
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? MSGFONTID_MYTIME : MSGFONTID_YOURTIME]);
                AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s", szFinalTimestamp);
            } else if(!bHideNick) {
                if (dat->isHistory)
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? H_MSGFONTID_MYNAME : H_MSGFONTID_YOURNAME]);
                else
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " %s ", rtfFonts[isSent ? MSGFONTID_MYNAME : MSGFONTID_YOURNAME]);
                AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s", szName);
            }
        }
	}
// XXX

// XXX underline
	if(dat->dwFlags & MWF_LOG_UNDERLINE && dbei.eventType != EVENTTYPE_STATUSCHANGE && dbei.eventType != EVENTTYPE_ERRMSG)
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\ul0");
// XXX end mod
    if (showColon)
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, ": ");
    else
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " ");
        
// XXX mod: show events in a new line
	if(dat->dwFlags & MWF_LOG_NEWLINE && dbei.eventType != EVENTTYPE_STATUSCHANGE && dbei.eventType != EVENTTYPE_ERRMSG && g_groupBreak == TRUE)
		AppendToBufferWithRTF(&buffer,&bufferEnd,&bufferAlloced,"\r\n");
// end mod

    switch (dbei.eventType) {
        case EVENTTYPE_MESSAGE:
        case EVENTTYPE_STATUSCHANGE:
        case EVENTTYPE_ERRMSG:
        {
#if defined( _UNICODE )
            wchar_t *msg;
#else
            BYTE *msg;
#endif
            if(dbei.eventType == EVENTTYPE_STATUSCHANGE || dbei.eventType == EVENTTYPE_ERRMSG) {
                if(dbei.eventType == EVENTTYPE_ERRMSG && dbei.cbBlob == 0)
                    break;
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? H_MSGFONTID_STATUSCHANGES : H_MSGFONTID_STATUSCHANGES]);
            }
            else {
                if (dat->isHistory)
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? H_MSGFONTID_MYMSG : H_MSGFONTID_YOURMSG]);
                else
                    AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? MSGFONTID_MYMSG : MSGFONTID_YOURMSG]);
            }
#if defined( _UNICODE )
            {
                int msglen = strlen((char *) dbei.pBlob) + 1;
                if(dbei.eventType == EVENTTYPE_MESSAGE && !isSent)
                    dat->stats.lastReceivedChars = msglen - 1;
                if (dbei.cbBlob >= (DWORD)(3 * msglen)) {         // FIXME!!! possible unicode issue?
                    msg = (TCHAR *) & dbei.pBlob[msglen];
                    //if(wcslen(msg) == (msglen - 1) && msg[msglen - 1] == (wchar_t)0x000) {
                        if(dat->dwEventIsShown & MWF_SHOW_EMPTYLINEFIX)
                            TrimMessage(msg);
                        AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, msg);
                    //}
                }
                else {
#ifdef __MATHMOD_SUPPORT
                    if (ServiceExists(MTH_GET_RTF_BITMAPTEXT))
                    {
                        char *rtfBmp = 0;
                        rtfBmp = (char*)CallService(MTH_GET_RTF_BITMAPTEXT,0, (LPARAM)dbei.pBlob);
                        //MessageBoxA(0,rtfBmp,"ding",0);
                        if ((rtfBmp != 0) && ((int)rtfBmp != CALLSERVICE_NOTFOUND)) {
//                            msg = (TCHAR *)alloca(sizeof(TCHAR) * (strlen(rtfBmp) + 1));
//                            MultiByteToWideChar(CP_ACP, 0, (char *) rtfBmp, -1, msg, (strlen(rtfBmp) + 1));
                            AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "{\\uc1%s}", rtfBmp);
                        }
                        CallService(MTH_FREE_RTF_BITMAPTEXT,0,(LPARAM) rtfBmp); // free the rtfBmp
                    }
                    else  //no MathModule installed - normal handling of message; without math.
                    {
                        msg = (TCHAR *) alloca(sizeof(TCHAR) * msglen);
                        MultiByteToWideChar(CP_ACP, 0, (char *) dbei.pBlob, -1, msg, msglen);
                        AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, msg);
                    }
#else       // mathmod
                    msg = (TCHAR *) alloca(sizeof(TCHAR) * msglen);
                    MultiByteToWideChar(dat->codePage, 0, (char *) dbei.pBlob, -1, msg, msglen);
                    if(dat->dwEventIsShown & MWF_SHOW_EMPTYLINEFIX)
                        TrimMessage(msg);
                    AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, msg);
#endif      // mathmod
                }
            }
#else   // unicode
            msg = (BYTE *) dbei.pBlob;
            if(dbei.eventType == EVENTTYPE_MESSAGE && !isSent)
                dat->stats.lastReceivedChars = lstrlenA(msg);
#ifdef __MATHMOD_SUPPORT
            //mathMod begin
			// show Math-Formula in Log
			if (ServiceExists(MTH_GET_RTF_BITMAPTEXT))
			{
				char* rtfBmp=0;
				rtfBmp=(char*)CallService(MTH_GET_RTF_BITMAPTEXT,0, (LPARAM)dbei.pBlob);
				//MessageBoxA(0,rtfBmp,"ding",0);
				if ((rtfBmp!=0) && ((int)rtfBmp!=CALLSERVICE_NOTFOUND))
					AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced,rtfBmp);
				CallService(MTH_FREE_RTF_BITMAPTEXT,0,(LPARAM) rtfBmp); // free the rtfBmp
			}
			else  //no MathModule installed - normal handling of message; without math.
			{
                AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s", msg);
			}

			// mathMod end
#else       // mathmod
            if(dat->dwEventIsShown & MWF_SHOW_EMPTYLINEFIX)
                TrimMessage(msg);
            AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s", msg);
#endif      // mathmod

#endif      // unicode

            if(dbei.eventType == EVENTTYPE_ERRMSG) {
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s\\line", rtfFonts[H_MSGFONTID_DIVIDERS]);
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, szDivider);
            }
            break;
        }
        case EVENTTYPE_URL:
            if (dat->isHistory)
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? H_MSGFONTID_MYMISC : H_MSGFONTID_YOURMISC]);
            else
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? MSGFONTID_MYMISC : MSGFONTID_YOURMISC]);
            AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s", dbei.pBlob);
            if ((dbei.pBlob + lstrlenA(dbei.pBlob) + 1) != NULL && lstrlenA(dbei.pBlob + lstrlenA(dbei.pBlob) + 1))
                AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, " (%s)", dbei.pBlob + lstrlenA(dbei.pBlob) + 1);
            break;
        case EVENTTYPE_FILE:
            if (dat->isHistory)
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? H_MSGFONTID_MYMISC : H_MSGFONTID_YOURMISC]);
            else
                AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", rtfFonts[isSent ? MSGFONTID_MYMISC : MSGFONTID_YOURMISC]);
            if ((dbei.pBlob + sizeof(DWORD) + lstrlenA(dbei.pBlob + sizeof(DWORD)) + 1) != NULL && lstrlenA(dbei.pBlob + sizeof(DWORD) + lstrlenA(dbei.pBlob + sizeof(DWORD)) + 1))
                AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s (%s)", dbei.pBlob + sizeof(DWORD), dbei.pBlob + sizeof(DWORD) + lstrlenA(dbei.pBlob + sizeof(DWORD)) + 1);
            else
                AppendToBufferWithRTF(&buffer, &bufferEnd, &bufferAlloced, "%s", dbei.pBlob + sizeof(DWORD));
            break;
    }

    if(dat->dwEventIsShown & MWF_SHOW_MICROLF)
        AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s\\par\\sl-1%s", rtfFonts[MSGDLGFONTCOUNT], rtfFonts[MSGDLGFONTCOUNT]);
    
    /* OnO: highlight end */
    //if(dbei.eventType == EVENTTYPE_MESSAGE && dat->dwFlags & MWF_LOG_INDIVIDUALBKG)
      //  AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight0");
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
                        dat->buffer = CreateRTFFromDbEvent(dat->dlgDat, dat->hContact, dat->hDbEvent, !dat->isEmpty, dat);
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
    
    // separator strings used for grid lines, message separation and so on...
    
    strcpy(szSep0, fAppend ? "\\par%s\\sl-1" : ((dat->dwEventIsShown & MWF_SHOW_MICROLF) ? "%s\\sl-1" : "\\par%s\\sl-1"));
    _snprintf(szSep1, 151, "\\highlight%s \\par\\sl0%s", "%d", rtfFonts[H_MSGFONTID_YOURTIME]);

    strcpy(szSep2, fAppend ? "\\par\\sl0" : ((dat->dwEventIsShown & MWF_SHOW_MICROLF) ? "\\sl1000" : "\\par\\sl1000"));

    ZeroMemory(&ci, sizeof(ci));
    ci.cbSize = sizeof(ci);
    ci.hContact = NULL;
    ci.szProto = dat->szProto;
    ci.dwFlag = CNF_DISPLAY;
    if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
        // CNF_DISPLAY always returns a string type
        szMyName = ci.pszVal;
    }
    else
        szMyName = NULL;
    szYourName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) dat->hContact, 0);
    
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
    
    if (fAppend && (dat->dwEventIsShown & MWF_SHOW_MICROLF)) {
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

#if defined(_STREAMTHREADING)
    if(g_StreamThreadRunning) {
        if(!fAppend) {
            SendMessage(hwndrtf, WM_SETREDRAW, TRUE, 0);
        }
        EnterCriticalSection(&sjcs);
        StreamJobs[g_StreamJobCurrent].dat = dat;
        StreamJobs[g_StreamJobCurrent].hwndOwner = hwndDlg;
        StreamJobs[g_StreamJobCurrent].startAt = startAt;
        StreamJobs[g_StreamJobCurrent].fAppend = fAppend;
        g_StreamJobCurrent++;
        dat->pendingStream++;
        dat->pContainer->pendingStream++;
        LeaveCriticalSection(&sjcs);
        ResumeThread(g_hStreamThread);
    }
    else
        ReplaceIcons(hwndDlg, dat, startAt, fAppend);
#else
    ReplaceIcons(hwndDlg, dat, startAt, fAppend);
#endif    
    
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
    char trbuffer[20];
    tr.lpstrText = trbuffer;

    hwndrtf = GetDlgItem(hwndDlg, IDC_LOG);
    fi.chrg.cpMin = startAt;

#if defined(_STREAMTHREADING)
    if(!fAppend && g_StreamThreadRunning) {
        InvalidateRect(hwndrtf, NULL, FALSE);
        SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 1, 0);
        SendMessage(hwndrtf, WM_SETREDRAW, FALSE, 0);
    }
#endif    
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
            
            //SendMessage(hwndrtf, EM_SETSEL, fi.chrgText.cpMin, fi.chrgText.cpMax + 2);
            tr.chrg.cpMin = fi.chrgText.cpMin + 3;
            tr.chrg.cpMax = fi.chrgText.cpMin + 5;
            SendMessageA(hwndrtf, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
            if((BYTE)(trbuffer[0] - '0') >= NR_LOGICONS ) {
                fi.chrg.cpMin = fi.chrgText.cpMax + 6;
                continue;
            }
            bIconIndex = ((BYTE)trbuffer[0] - (BYTE)'0') * 3;
            if(dat->dwFlags & MWF_LOG_INDIVIDUALBKG) {
                if(trbuffer[1] == '<')
                    bIconIndex += 1;
                else
                    bIconIndex += 2;
            }

#if defined(_STREAMTHREADING)
            if(g_StreamThreadRunning)
                SendMessage(hwndDlg, DM_INSERTICON, (WPARAM)ole, (LPARAM)msgLogIcons[bIconIndex].hBmp);
            else
                ImageDataInsertBitmap(ole, msgLogIcons[bIconIndex].hBmp);
#else
            ImageDataInsertBitmap(ole, msgLogIcons[bIconIndex].hBmp);
#endif            
            fi.chrg.cpMin = cr.cpMax + 6;
        }
        ReleaseRichEditOle(ole);
    }

    /*
     * do smiley replacing, using the service
     */

    if(g_SmileyAddAvail && DBGetContactSettingByte(NULL, "SmileyAdd", "PluginSupportEnabled", 0)) {
        CHARRANGE sel;
        SMADD_RICHEDIT2 smadd;

        sel.cpMin = startAt;
        sel.cpMax = -1;

        smadd.cbSize = sizeof(smadd);
        smadd.hwndRichEditControl = GetDlgItem(hwndDlg, IDC_LOG);
        smadd.Protocolname = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)dat->hContact, 0);
        if(startAt > 0)
            smadd.rangeToReplace = &sel;
        else
            smadd.rangeToReplace = NULL;
        smadd.disableRedraw = TRUE;
        if(dat->doSmileys) {
#if defined(_STREAMTHREADING)
            if(g_StreamThreadRunning)
                CallService(MS_SMILEYADD_REPLACESMILEYS, TABSRMM_SMILEYADD_BKGCOLORMODE | TABSRMM_SMILEYADD_THREADING, (LPARAM)&smadd);
            else
                CallService(MS_SMILEYADD_REPLACESMILEYS, TABSRMM_SMILEYADD_BKGCOLORMODE, (LPARAM)&smadd);
#else
            CallService(MS_SMILEYADD_REPLACESMILEYS, TABSRMM_SMILEYADD_BKGCOLORMODE, (LPARAM)&smadd);
#endif            
        }
    }
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
	static struct { UINT cpId; char *cpName; } cpTable[] = {
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
		{	1361,	"Korean (Johab)" }
	};

    cp = atoi(str);
	count = sizeof(cpTable)/sizeof(cpTable[0]);
	for (i=0; i<count && cpTable[i].cpId!=cp; i++);
	if (i < count) {
        AppendMenuA(g_hMenuEncoding, MF_STRING, cp, Translate(cpTable[i].cpName));
	}
	return TRUE;
}

void BuildCodePageList()
{
    g_hMenuEncoding = CreateMenu();
    AppendMenuA(g_hMenuEncoding, MF_STRING, 500, Translate("Use default codepage"));
    AppendMenuA(g_hMenuEncoding, MF_SEPARATOR, 0, 0);
    EnumSystemCodePagesA(LangAddCallback, CP_INSTALLED);
}

static char *MakeRelativeDate(struct MessageWindowData *dat, time_t check, int groupBreak)
{
    static char szResult[512];
    char str[80];
    
    DBTIMETOSTRING dbtts;

    struct tm tm_now, tm_today;
    time_t now = time(NULL);
    time_t today;

    dbtts.cbDest = 70;;
    dbtts.szDest = str;
    
    if(!groupBreak || !(dat->dwFlags & MWF_LOG_SHOWDATES)) {
        dbtts.szFormat = (dat->dwFlags & MWF_LOG_SHOWSECONDS) ? "s" : "t";
        szResult[0] = '\0';
    }
    else {
        tm_now = *localtime(&now);
        tm_today = tm_now;
        tm_today.tm_hour = tm_today.tm_min = tm_today.tm_sec = 0;
        today = mktime(&tm_today);

        if(dat->dwFlags & MWF_LOG_USERELATIVEDATES && check >= today) {
            dbtts.szFormat = (dat->dwFlags & MWF_LOG_SHOWSECONDS) ? "s" : "t";
            strcpy(szResult, szToday);
        }
        else if(dat->dwFlags & MWF_LOG_USERELATIVEDATES && check > (today - 86400)) {
            dbtts.szFormat = (dat->dwFlags & MWF_LOG_SHOWSECONDS) ? "s" : "t";
            strcpy(szResult, szYesterday);
        }
        else {
            if(dat->dwFlags & MWF_LOG_LONGDATES)
                dbtts.szFormat = (dat->dwFlags & MWF_LOG_SHOWSECONDS) ? "D s" : "D t";
            else
                dbtts.szFormat = (dat->dwFlags & MWF_LOG_SHOWSECONDS) ? "d s" : "d t";
            szResult[0] = '\0';
        }
    }
	CallService(MS_DB_TIME_TIMESTAMPTOSTRING, check, (LPARAM) & dbtts);
    strncat(szResult, str, 500);
    return szResult;
}   

/*
 * decodes UTF-8 to unicode
 * taken from jabber protocol implementation and slightly modified
 * return value is static
 */

WCHAR *Utf8Decode(const char *str)
{
	int i, len;
	char *p;
	static WCHAR *wszTemp;

	if (str == NULL) return NULL;

	len = strlen(str);

    if ((wszTemp = (WCHAR *) realloc(wszTemp, sizeof(WCHAR) * (len + 1))) == NULL)
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
	wszTemp[i] = '\0';

	return wszTemp;
}

/*
 * convert unicode to UTF-8
 * code taken from jabber protocol implementation and slightly modified.
 * return value is static
 */

char *Utf8Encode(const WCHAR *str)
{
	static unsigned char *szOut;
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

	if ((szOut = (unsigned char *) realloc(szOut, len + 1)) == NULL)
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

