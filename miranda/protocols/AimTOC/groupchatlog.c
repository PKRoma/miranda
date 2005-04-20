/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003-2005 Robert Rainwater

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
*/
#include "aim.h"


#define STREAMSTAGE_HEADER  0
#define STREAMSTAGE_EVENT   1
#define STREAMSTAGE_TAIL    2
#define STREAMSTAGE_STOP    3
struct LogStreamData
{
    int stage;
    char *buffer;
    int bufferOffset, bufferLen;
    TTOCChannelMessage *msgdat;
    DWORD charsOutput;
    int showTime;
    int showDate;
	HWND hwnd;
};
static int logPixelSY;

static char *aim_gchatlog_setstyle(int style)
{
    static char szStyle[128];
    LOGFONT lf;

    LoadGroupChatFont(style, &lf, NULL);
    mir_snprintf(szStyle, sizeof(szStyle), "\\f%u\\cf%u\\b%d\\i%d\\fs%u", style, style, lf.lfWeight >= FW_BOLD ? 1 : 0, lf.lfItalic,
              2 * abs(lf.lfHeight) * 74 / logPixelSY);
    return szStyle;
}

static void aim_gchatlog_appendtobuffer(char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
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

static int aim_gchatlog_appendtobufferwithrtf(char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
{
    va_list va;
    int charsDone, i;
    DWORD textCharsCount = 0;

    va_start(va, fmt);
    for (;;) {
        charsDone = mir_vsnprintf(*buffer + *cbBufferEnd, *cbBufferAlloced - *cbBufferEnd, fmt, va);
        if (charsDone >= 0)
            break;
        *cbBufferAlloced += 1024;
        *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
    }
    va_end(va);
    textCharsCount = _mbslen(*buffer + *cbBufferEnd);
    *cbBufferEnd += charsDone;
    for (i = *cbBufferEnd - charsDone; (*buffer)[i]; i++) {
        if ((*buffer)[i] == '\r' && (*buffer)[i + 1] == '\n') {
            textCharsCount--;
            if (*cbBufferEnd + 4 > *cbBufferAlloced) {
                *cbBufferAlloced += 1024;
                *buffer = (char *) realloc(*buffer, *cbBufferAlloced);
            }
            MoveMemory(*buffer + i + 5, *buffer + i + 2, *cbBufferEnd - i - 1);
            CopyMemory(*buffer + i, "\\par ", 5);
            *cbBufferEnd += 3;
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
    return textCharsCount;
}

static char *aim_gchatlog_creatertfheader()
{
    char *buffer;
    int bufferAlloced, bufferEnd;
    int i;
    LOGFONT lf;
    COLORREF colour;
    HDC hdc;

    hdc = GetDC(NULL);
    logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
    bufferEnd = 0;
    bufferAlloced = 1024;
    buffer = (char *) malloc(bufferAlloced);
    buffer[0] = '\0';
    aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "{\\rtf1\\ansi\\deff0{\\fonttbl");
    for (i = 0; i < msgDlgFontCount; i++) {
        LoadGroupChatFont(i, &lf, NULL);
        aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "{\\f%u\\fnil\\fcharset%u %s;}", i, lf.lfCharSet, lf.lfFaceName);
    }
    aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "}{\\colortbl ");
    for (i = 0; i < msgDlgFontCount; i++) {
        LoadGroupChatFont(i, NULL, &colour);
        aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour),
                                    GetBValue(colour));
    }
    if (GetSysColorBrush(COLOR_HOTLIGHT) == NULL)
        colour = RGB(0, 0, 255);
    else
        colour = GetSysColor(COLOR_HOTLIGHT);
    aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour),
                                GetBValue(colour));
    aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "}\\pard");
    return buffer;
}

static char *aim_gchatlog_creatertftail()
{
    char *buffer;
    int bufferAlloced, bufferEnd;

    bufferEnd = 0;
    bufferAlloced = 1024;
    buffer = (char *) malloc(bufferAlloced);
    buffer[0] = '\0';
    aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "}");
    return buffer;
}

static char *aim_gchatlog_creatertfbuffer(struct LogStreamData *streamData)
{
    char *buffer, log[512];
    int bufferAlloced, bufferEnd, me = 0;

    bufferEnd = 0;
    bufferAlloced = 1024;
    buffer = (char *) malloc(bufferAlloced);
    buffer[0] = '\0';

    if (streamData->msgdat->dwType == AIM_GCHAT_CHANPART || streamData->msgdat->dwType == AIM_GCHAT_CHANJOIN) {
        if (streamData->showTime) {
            DBTIMETOSTRING dbtts;
            char str[64];

            dbtts.szFormat = streamData->showDate ? "d t" : "t";
            dbtts.cbDest = sizeof(str);
            dbtts.szDest = str;
            CallService(MS_DB_TIME_TIMESTAMPTOSTRING, (WPARAM) time(NULL), (LPARAM) & dbtts);
            aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", aim_gchatlog_setstyle(GCHATFONTID_DATE));
            streamData->charsOutput += aim_gchatlog_appendtobufferwithrtf(&buffer, &bufferEnd, &bufferAlloced, "[%s] ", str);
			mir_snprintf(log, sizeof(log), "[%s] ", str);
			aim_gchat_logchat(streamData->msgdat->szRoom, log);
        }
        aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", aim_gchatlog_setstyle(GCHATFONTID_MYPARTS));
        mir_snprintf(log, sizeof(log), Translate("*** %s has %s %s"), streamData->msgdat->szUser,
                  streamData->msgdat->dwType == AIM_GCHAT_CHANPART ? Translate("left") : Translate("joined"), streamData->msgdat->szRoom);
        aim_gchat_logchat(streamData->msgdat->szRoom, log);
        aim_gchat_logchat(streamData->msgdat->szRoom, "\n");
        streamData->charsOutput +=
            aim_gchatlog_appendtobufferwithrtf(&buffer, &bufferEnd, &bufferAlloced, Translate("*** %s has %s %s"), streamData->msgdat->szUser,
                                               streamData->msgdat->dwType == AIM_GCHAT_CHANPART ? Translate("left") : Translate("joined"),
                                               streamData->msgdat->szRoom);
        aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "\\par");
        streamData->charsOutput++;
    }
    else if (streamData->msgdat->dwType == AIM_GCHAT_CHANMESS) {
        char *msg = streamData->msgdat->msg->szMessage;

        if (aim_util_isme(streamData->msgdat->msg->szUser))
            me = 1;
        if (streamData->showTime) {
            DBTIMETOSTRING dbtts;
            char str[64];

            dbtts.szFormat = streamData->showDate ? "d t" : "t";
            dbtts.cbDest = sizeof(str);
            dbtts.szDest = str;
            CallService(MS_DB_TIME_TIMESTAMPTOSTRING, (WPARAM) time(NULL), (LPARAM) & dbtts);
            aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ",
                                        aim_gchatlog_setstyle(me ? GCHATFONTID_MYDATE : GCHATFONTID_DATE));
            streamData->charsOutput += aim_gchatlog_appendtobufferwithrtf(&buffer, &bufferEnd, &bufferAlloced, "[%s] ", str);
			mir_snprintf(log, sizeof(log), "[%s] ", str);
			aim_gchat_logchat(streamData->msgdat->szRoom, log);
        }
        aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", aim_gchatlog_setstyle(me ? GCHATFONTID_MYNAME : GCHATFONTID_NAME));
        mir_snprintf(log, sizeof(log), "<%s> %s", streamData->msgdat->msg->szUser, msg);
        aim_gchat_logchat(streamData->msgdat->szRoom, log);
        aim_gchat_logchat(streamData->msgdat->szRoom, "\n");
        streamData->charsOutput += aim_gchatlog_appendtobufferwithrtf(&buffer, &bufferEnd, &bufferAlloced, " <%s> ", streamData->msgdat->msg->szUser);
        aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", aim_gchatlog_setstyle(me ? GCHATFONTID_MYMSG : GCHATFONTID_MSG));
        streamData->charsOutput += aim_gchatlog_appendtobufferwithrtf(&buffer, &bufferEnd, &bufferAlloced, "%s", msg);
        aim_gchatlog_appendtobuffer(&buffer, &bufferEnd, &bufferAlloced, "\\par");
        streamData->charsOutput++;
    }
    return buffer;
}

static DWORD CALLBACK aim_gchatlog_logstreaminevent(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
    struct LogStreamData *dat = (struct LogStreamData *) dwCookie;
    if (dat->buffer == NULL) {
        dat->bufferOffset = 0;
        switch (dat->stage) {
            case STREAMSTAGE_HEADER:
                dat->buffer = aim_gchatlog_creatertfheader();
                dat->stage = STREAMSTAGE_EVENT;
                break;
            case STREAMSTAGE_EVENT:
                dat->buffer = aim_gchatlog_creatertfbuffer(dat);
                dat->stage = STREAMSTAGE_TAIL;
                break;
            case STREAMSTAGE_TAIL:
                dat->buffer = aim_gchatlog_creatertftail();
                dat->stage = STREAMSTAGE_STOP;
                break;
            case STREAMSTAGE_STOP:
                *pcb = 0;
                return 0;
        }
        dat->bufferLen = lstrlen(dat->buffer);
    }
    *pcb = min(cb, dat->bufferLen - dat->bufferOffset);
    CopyMemory(pbBuff, dat->buffer + dat->bufferOffset, *pcb);
    dat->bufferOffset += *pcb;
    if (dat->bufferOffset == dat->bufferLen) {
        free(dat->buffer);
        dat->buffer = NULL;
    }
	if (ServiceExists(MS_SMILEYADD_REPLACESMILEYS)) {
		SMADD_RICHEDIT2 sm;

		ZeroMemory(&sm, sizeof(sm));
		sm.cbSize = sizeof(sm);
		sm.hwndRichEditControl = dat->hwnd;
		sm.Protocolname = AIM_PROTO;
		sm.rangeToReplace = NULL;
		CallService(MS_SMILEYADD_REPLACESMILEYS, 0, (LPARAM)&sm);
	}
    return 0;
}

void aim_gchatlog_streaminevent(HWND hwndDlg, TTOCChannelMessage msg)
{
    EDITSTREAM stream;
    struct LogStreamData streamData;
    CHARRANGE oldSel, sel;

    ZeroMemory(&stream, sizeof(stream));
    ZeroMemory(&streamData, sizeof(streamData));
    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_HIDESELECTION, TRUE, 0);
    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXGETSEL, 0, (LPARAM) & oldSel);
	streamData.hwnd = GetDlgItem(hwndDlg, IDC_LOG);
    streamData.msgdat = &msg;
    stream.pfnCallback = aim_gchatlog_logstreaminevent;
    stream.dwCookie = (DWORD) & streamData;
    streamData.showTime = (int) DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GD, AIM_KEY_GD_DEF);
    streamData.showDate = streamData.showTime && DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GY, AIM_KEY_GY_DEF);
    sel.cpMin = sel.cpMax = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LOG));
    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXSETSEL, 0, (LPARAM) & sel);
    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXGETSEL, 0, (LPARAM) & sel);
    streamData.charsOutput = sel.cpMin;
    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_STREAMIN, SFF_SELECTION | SF_RTF, (LPARAM) & stream);
    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXSETSEL, 0, (LPARAM) & oldSel);
    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_HIDESELECTION, FALSE, 0);
    if (GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG), GWL_STYLE) & WS_VSCROLL)
        PostMessage(hwndDlg, GC_SCROLLLOGTOBOTTOM, 0, 0);
}
