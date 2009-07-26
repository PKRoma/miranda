/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

Chat module plugin for Miranda IM

Copyright (C) 2003 J�rgen Persson

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

log.c implements the group chat message history display using a
rich edit text control.

$Id: log.c 10402 2009-07-24 00:35:21Z silvercircle $
*/

#include "../src/commonheaders.h"
#include <math.h>
#include <mbstring.h>
#include <shlwapi.h>

//#include "../m_MathModule.h"


/*
 * The code for streaming the text is to a large extent copied from
 * the srmm module and then modified to fit the chat module.
 */

extern FONTINFO	aFonts[OPTIONS_FONTCOUNT];
extern HICON		hIcons[30];

static PBYTE		pLogIconBmpBits[14];
static int			logIconBmpSize[ SIZEOF(pLogIconBmpBits)];

static int			logPixelSY = 0;
static int			logPixelSX = 0;
static char		*szDivider = "\\strike----------------------------------------------------------------------------\\strike0";
static char		CHAT_rtfFontsGlobal[OPTIONS_FONTCOUNT + 2][RTFCACHELINESIZE];
static char		*CHAT_rtffonts = 0;

void GetIconSize(HICON hIcon, int* sizeX, int* sizeY);

static int EventToIndex(LOGINFO * lin)
{
	switch (lin->iType) {
		case GC_EVENT_MESSAGE:
			if (lin->bIsMe)
				return 10;
			else
				return 9;

		case GC_EVENT_JOIN:
			return 3;
		case GC_EVENT_PART:
			return 4;
		case GC_EVENT_QUIT:
			return 5;
		case GC_EVENT_NICK:
			return 7;
		case GC_EVENT_KICK:
			return 6;
		case GC_EVENT_NOTICE:
			return 8;
		case GC_EVENT_TOPIC:
			return 11;
		case GC_EVENT_INFORMATION:
			return 12;
		case GC_EVENT_ADDSTATUS:
			return 13;
		case GC_EVENT_REMOVESTATUS:
			return 14;
		case GC_EVENT_ACTION:
			return 15;
	}
	return 0;
}

static BYTE EventToSymbol(LOGINFO *lin)
{
	switch (lin->iType) {
		case GC_EVENT_MESSAGE:
			return (lin->bIsMe) ? 0x37 : 0x38;
		case GC_EVENT_JOIN:
			return 0x34;
		case GC_EVENT_PART:
			return 0x33;
		case GC_EVENT_QUIT:
			return 0x39;
		case GC_EVENT_NICK:
			return 0x71;
		case GC_EVENT_KICK:
			return 0x72;
		case GC_EVENT_NOTICE:
			return 0x28;
		case GC_EVENT_INFORMATION:
			return 0x69;
		case GC_EVENT_ADDSTATUS:
			return 0x35;
		case GC_EVENT_REMOVESTATUS:
			return 0x36;
		case GC_EVENT_ACTION:
			return 0x60;
	}
	return 0x73;
}

static int EventToIcon(LOGINFO * lin)
{
	switch (lin->iType) {
		case GC_EVENT_MESSAGE:
			if (lin->bIsMe)
				return ICON_MESSAGEOUT;
			else
				return ICON_MESSAGE;

		case GC_EVENT_JOIN:
			return ICON_JOIN;
		case GC_EVENT_PART:
			return ICON_PART;
		case GC_EVENT_QUIT:
			return ICON_QUIT;
		case GC_EVENT_NICK:
			return ICON_NICK;
		case GC_EVENT_KICK:
			return ICON_KICK;
		case GC_EVENT_NOTICE:
			return ICON_NOTICE;
		case GC_EVENT_TOPIC:
			return ICON_TOPIC;
		case GC_EVENT_INFORMATION:
			return ICON_INFO;
		case GC_EVENT_ADDSTATUS:
			return ICON_ADDSTATUS;
		case GC_EVENT_REMOVESTATUS:
			return ICON_REMSTATUS;
		case GC_EVENT_ACTION:
			return ICON_ACTION;
	}
	return 0;
}

/* replace pattern `ptrn' with the string `rplc' in string `src' points to */
static TCHAR * _tcsrplc(TCHAR **src, const TCHAR *ptrn, const TCHAR *rplc)
{
	size_t lSrc, lPtrn, lRplc;
	TCHAR *tszFound, *tszTail;

	lSrc = _tcslen(*src);
	lPtrn = _tcslen(ptrn);
	lRplc = _tcslen(rplc);
	if (lPtrn && lSrc && lSrc >= lPtrn && (tszFound = _tcsstr(*src, ptrn)) != NULL) {
		if (lRplc > lPtrn)
			*src = (TCHAR *) realloc((void *) * src,
									 sizeof(TCHAR) * (lSrc + lRplc - lPtrn + 1));
		if (tszTail = (TCHAR *) malloc(sizeof(TCHAR) *
									   (lSrc - (tszFound - *src) - lPtrn + 1))) {
			/* save tail */
			_tcscpy(tszTail, tszFound + lPtrn);
			/* write replacement string */
			_tcscpy(tszFound, rplc);
			/* write tail */
			_tcscpy(tszFound + lRplc, tszTail);
			free((void *) tszTail);
		}
	}
	return *src;
}

/*
 * replace pattern `ptrn' with the string `rplc' in string `src',
 * `src' is supposed to be `n' character long (or no checking is done if n < 0).
 * This function is useful for statically allocated buffers
 */
static TCHAR * _tcsnrplc(TCHAR *src, size_t n, const TCHAR *ptrn, const TCHAR *rplc)
{
	size_t lSrc, lPtrn, lRplc;
	TCHAR *tszFound, *tszTail;

	lSrc = _tcslen(src);
	lPtrn = _tcslen(ptrn);
	lRplc = _tcslen(rplc);
	if (lPtrn && lSrc && lSrc >= lPtrn && /* lengths are ok */
			(tszFound = _tcsstr(src, ptrn)) != NULL && /* pattern was found in string */
			(n < 0 || lSrc - lPtrn + lRplc < n) && /* there is enough room in the string */
			(tszTail = (TCHAR *) malloc(sizeof(TCHAR) *
										(lSrc - (tszFound - src) - lPtrn + 1))) != NULL) {
		/* save tail */
		_tcscpy(tszTail, tszFound + lPtrn);
		/* write replacement string */
		_tcscpy(tszFound, rplc);
		/* write tail */
		_tcscpy(tszFound + lRplc, tszTail);
		free((void *) tszTail);
	}
	return src;
}

static char *Log_SetStyle(int style, int fontindex)
{
	if (style < OPTIONS_FONTCOUNT)
		return CHAT_rtffonts + (style * RTFCACHELINESIZE);

	return "";
}

static void Log_Append(char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
{
	va_list va;
	int charsDone = 0;

	va_start(va, fmt);
	for (;;) {
		charsDone = mir_vsnprintf(*buffer + *cbBufferEnd, *cbBufferAlloced - *cbBufferEnd, fmt, va);
		if (charsDone >= 0)
			break;
		*cbBufferAlloced += 4096;
		*buffer = (char *) mir_realloc(*buffer, *cbBufferAlloced);
	}
	va_end(va);
	*cbBufferEnd += charsDone;
}

static int Log_AppendRTF(LOGSTREAMDATA* streamData, BOOL simpleMode, char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const TCHAR *fmt, ...)
{
	va_list va;
	int lineLen, textCharsCount = 0;
	TCHAR* line = (TCHAR*)alloca(8001 * sizeof(TCHAR));
	char* d;

	va_start(va, fmt);
	lineLen = _vsntprintf(line, 8000, fmt, va);
	if (lineLen < 0)
		return 0;
	line[lineLen] = 0;
	va_end(va);

	lineLen = lineLen * 20 + 8;
	if (*cbBufferEnd + lineLen > *cbBufferAlloced) {
		cbBufferAlloced[0] += (lineLen + 1024 - lineLen % 1024);
		if ((d = (char *) mir_realloc(*buffer, *cbBufferAlloced)) == 0)
			return 0;
		*buffer = d;
	}

	d = *buffer + *cbBufferEnd;

	for (; *line; line++, textCharsCount++) {
		if (*line == '\r' && line[1] == '\n') {
			CopyMemory(d, "\\par ", 5);
			line++;
			d += 5;
		} else if (*line == '\n') {
			CopyMemory(d, "\\line ", 6);
			d += 6;
		} else if (*line == '%' && !simpleMode) {
			char szTemp[200];

			szTemp[0] = '\0';
			switch (*++line) {
				case '\0':
				case '%':
					*d++ = '%';
					break;

				case 'c':
				case 'f':
					if (g_Settings.StripFormat || streamData->bStripFormat)
						line += 2;

					else if (line[1] != '\0' && line[2] != '\0') {
						TCHAR szTemp3[3], c = *line;
						int col;
						szTemp3[0] = line[1];
						szTemp3[1] = line[2];
						szTemp3[2] = '\0';
						line += 2;

						col = _ttoi(szTemp3);
						col += (OPTIONS_FONTCOUNT + 1);
						mir_snprintf(szTemp, SIZEOF(szTemp), (c == 'c') ? "\\cf%u " : "\\highlight%u ", col);
					}
					break;
				case 'C':
				case 'F':
					if (!g_Settings.StripFormat && !streamData->bStripFormat) {
						int j = streamData->lin->bIsHighlighted ? 16 : EventToIndex(streamData->lin);
						if (*line == 'C')
							mir_snprintf(szTemp, SIZEOF(szTemp), "\\cf%u ", j + 1);
						else
							mir_snprintf(szTemp, SIZEOF(szTemp), "\\highlight0 ");
					}
					break;
				case 'b':
				case 'u':
				case 'i':
					if (!streamData->bStripFormat)
						mir_snprintf(szTemp, SIZEOF(szTemp), (*line == 'u') ? "\\%cl " : "\\%c ", *line);
					break;

				case 'B':
				case 'U':
				case 'I':
					if (!streamData->bStripFormat) {
						mir_snprintf(szTemp, SIZEOF(szTemp), (*line == 'U') ? "\\%cl0 " : "\\%c0 ", *line);
						CharLowerA(szTemp);
					}
					break;

				case 'r':
					if (!streamData->bStripFormat) {
						int index = EventToIndex(streamData->lin);
						mir_snprintf(szTemp, SIZEOF(szTemp), "%s ", Log_SetStyle(index, index));
					}
					break;
			}

			if (szTemp[0]) {
				int iLen = lstrlenA(szTemp);
				memcpy(d, szTemp, iLen);
				d += iLen;
			}
		} else if (*line == '\t' && !streamData->bStripFormat) {
			CopyMemory(d, "\\tab ", 5);
			d += 5;
		} else if ((*line == '\\' || *line == '{' || *line == '}') && !streamData->bStripFormat) {
			*d++ = '\\';
			*d++ = (char) * line;
		} else if (*line > 0 && *line < 128) {
			*d++ = (char) * line;
		}
#if defined( _UNICODE )
		else d += sprintf(d, "\\u%u ?", (WORD) * line);
#else
		else d += sprintf(d, "\\'%02x", (BYTE) * line);
#endif
	}

	*cbBufferEnd = (int)(d - *buffer);
	return textCharsCount;
}

static void AddEventToBuffer(char **buffer, int *bufferEnd, int *bufferAlloced, LOGSTREAMDATA *streamData)
{
	TCHAR szTemp[512], szTemp2[512];
	TCHAR* pszNick = NULL;

	if (streamData == NULL)
		return;

	if (streamData->lin == NULL)
		return;

	if (streamData->lin->ptszNick) {
		if (g_Settings.LogLimitNames && lstrlen(streamData->lin->ptszNick) > 20) {
			lstrcpyn(szTemp, streamData->lin->ptszNick, 20);
			lstrcpyn(szTemp + 20, _T("..."), 4);
		} else lstrcpyn(szTemp, streamData->lin->ptszNick, 511);

		if (g_Settings.ClickableNicks)
			mir_sntprintf(szTemp2, SIZEOF(szTemp2), _T("~~++#%s#++~~"), szTemp);
		else
			_tcscpy(szTemp2, szTemp);

		if (streamData->lin->ptszUserInfo && streamData->lin->iType != GC_EVENT_TOPIC)
			mir_sntprintf(szTemp, SIZEOF(szTemp), _T("%s (%s)"), szTemp2, streamData->lin->ptszUserInfo);
		else
			mir_sntprintf(szTemp, SIZEOF(szTemp), _T("%s"), szTemp2);
		pszNick = szTemp;
	}

	switch (streamData->lin->iType) {
		case GC_EVENT_MESSAGE:
			if (streamData->lin->ptszText)
				Log_AppendRTF(streamData, FALSE, buffer, bufferEnd, bufferAlloced, _T("%s"), streamData->lin->ptszText);
			break;
		case GC_EVENT_ACTION:
			if (streamData->lin->ptszNick && streamData->lin->ptszText) {
				Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced, _T("%s "), streamData->lin->ptszNick);
				Log_AppendRTF(streamData, FALSE, buffer, bufferEnd, bufferAlloced, _T("%s"), streamData->lin->ptszText);
			}
			break;
		case GC_EVENT_JOIN:
			if (pszNick) {
				if (!streamData->lin->bIsMe)
					/* replace nick of a newcomer with a link */
					Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced, TranslateT("%s has joined"), pszNick);
				else
					Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced, TranslateT("You have joined %s"), streamData->si->ptszName);
			}
			break;
		case GC_EVENT_PART:
			if (pszNick)
				Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced, TranslateT("%s has left"), pszNick);
			if (streamData->lin->ptszText)
				Log_AppendRTF(streamData, FALSE, buffer, bufferEnd, bufferAlloced, _T(": %s"), streamData->lin->ptszText);
			break;
		case GC_EVENT_QUIT:
			if (pszNick)
				Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced, TranslateT("%s has disconnected"), pszNick);
			if (streamData->lin->ptszText)
				Log_AppendRTF(streamData, FALSE, buffer, bufferEnd, bufferAlloced, _T(": %s"), streamData->lin->ptszText);
			break;
		case GC_EVENT_NICK:
			if (pszNick && streamData->lin->ptszText) {
				if (!streamData->lin->bIsMe)
					Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced, TranslateT("%s is now known as %s"), pszNick, streamData->lin->ptszText);
				else
					Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced, TranslateT("You are now known as %s"), streamData->lin->ptszText);
			}
			break;
		case GC_EVENT_KICK:
			if (pszNick && streamData->lin->ptszStatus)
				Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced,
							  TranslateT("%s kicked %s"), streamData->lin->ptszStatus, pszNick);

			if (streamData->lin->ptszText)
				Log_AppendRTF(streamData, FALSE, buffer, bufferEnd, bufferAlloced, _T(": %s"), streamData->lin->ptszText);
			break;
		case GC_EVENT_NOTICE:
			if (pszNick && streamData->lin->ptszText) {
				Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced, TranslateT("Notice from %s: "), pszNick);
				Log_AppendRTF(streamData, FALSE, buffer, bufferEnd, bufferAlloced, _T("%s"), streamData->lin->ptszText);
			}
			break;
		case GC_EVENT_TOPIC:
			if (streamData->lin->ptszText)
				Log_AppendRTF(streamData, FALSE, buffer, bufferEnd, bufferAlloced, TranslateT("The topic is \'%s%s\'"), streamData->lin->ptszText, _T("%r"));
			if (pszNick)
				Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced,
							  (streamData->lin->ptszUserInfo) ? TranslateT(" (set by %s on %s)") : TranslateT(" (set by %s)"),
							  pszNick, streamData->lin->ptszUserInfo);
			break;
		case GC_EVENT_INFORMATION:
			if (streamData->lin->ptszText)
				Log_AppendRTF(streamData, FALSE, buffer, bufferEnd, bufferAlloced, (streamData->lin->bIsMe) ? _T("--> %s") : _T("%s"), streamData->lin->ptszText);
			break;
		case GC_EVENT_ADDSTATUS:
			if (pszNick && streamData->lin->ptszText && streamData->lin->ptszStatus)
				Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced,
							  TranslateT("%s enables \'%s\' status for %s"),
							  streamData->lin->ptszText, streamData->lin->ptszStatus, pszNick);
			break;
		case GC_EVENT_REMOVESTATUS:
			if (pszNick && streamData->lin->ptszText && streamData->lin->ptszStatus) {
				Log_AppendRTF(streamData, TRUE, buffer, bufferEnd, bufferAlloced,
							  TranslateT("%s disables \'%s\' status for %s"),
							  streamData->lin->ptszText , streamData->lin->ptszStatus, pszNick);
			}
			break;
	}
}

TCHAR* MakeTimeStamp(TCHAR* pszStamp, time_t time)
{
	static TCHAR szTime[30];
	_tcsftime(szTime, 29, pszStamp, localtime(&time));
	return szTime;
}

static char* Log_CreateRTF(LOGSTREAMDATA *streamData)
{
	char *buffer, *header;
	int bufferAlloced, bufferEnd, i, me = 0;
	LOGINFO * lin = streamData->lin;
	MODULEINFO *mi = MM_FindModule(streamData->si->pszModule);

	// guesstimate amount of memory for the RTF
	bufferEnd = 0;
	bufferAlloced = streamData->bRedraw ? 2048 * (streamData->si->iEventCount + 2) : 2048;
	buffer = (char *) mir_alloc(bufferAlloced);
	buffer[0] = '\0';

	// ### RTF HEADER
	header = mi->pszHeader;
	streamData->crCount = mi->nColorCount;

	if (header)
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, header);


	// ### RTF BODY (one iteration per event that should be streamed in)
	while (lin) {
		// filter
		if (streamData->si->iType != GCW_CHATROOM || !streamData->si->bFilterEnabled || (streamData->si->iLogFilterFlags&lin->iType) != 0) {
			if (lin->next != NULL)
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\par ");

			if (streamData->dat->dwFlags & MWF_DIVIDERWANTED || lin->dwFlags & MWF_DIVIDERWANTED) {
				static char szStyle_div[128] = "\0";
				if (szStyle_div[0] == 0)
					mir_snprintf(szStyle_div, 128, "\\f%u\\cf%u\\ul0\\b%d\\i%d\\fs%u", 17, 18, 0, 0, 5);

				lin->dwFlags |= MWF_DIVIDERWANTED;
				if (lin->prev || !streamData->bRedraw)
					Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\qc\\sl-1\\highlight%d %s ---------------------------------------------------------------------------------------\\par ", 18, szStyle_div);
				streamData->dat->dwFlags &= ~MWF_DIVIDERWANTED;
			}
			// create new line, and set font and color
			Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\ql\\sl0%s ", Log_SetStyle(0, 0));
			Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\v~-+%d+-~\\v0 ", lin);

			// Insert icon
			if (g_Settings.LogSymbols)                // use symbols
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s %c", Log_SetStyle(17, 17), EventToSymbol(lin));
			else {
				if (lin->iType&g_Settings.dwIconFlags || lin->bIsHighlighted && g_Settings.dwIconFlags&GC_EVENT_HIGHLIGHT) {
					int iIndex = (lin->bIsHighlighted && g_Settings.dwIconFlags & GC_EVENT_HIGHLIGHT) ? ICON_HIGHLIGHT : EventToIcon(lin);
					Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\f0\\fs14");
					while (bufferAlloced - bufferEnd < (logIconBmpSize[0] + 20))
						bufferAlloced += 4096;
					buffer = (char *) mir_realloc(buffer, bufferAlloced);
					CopyMemory(buffer + bufferEnd, pLogIconBmpBits[iIndex], logIconBmpSize[iIndex]);
					bufferEnd += logIconBmpSize[iIndex];
				}
			}

			if (g_Settings.TimeStampEventColour) {
				// colored timestamps
				static char szStyle[256];
				int iii;
				if (lin->ptszNick && lin->iType == GC_EVENT_MESSAGE) {
					iii = lin->bIsHighlighted ? 16 : (lin->bIsMe ? 2 : 1);
					mir_snprintf(szStyle, SIZEOF(szStyle), "\\f0\\cf%u\\ul0\\highlight0\\b%d\\i%d\\ul%d\\fs%u", iii + 1, aFonts[0].lf.lfWeight >= FW_BOLD ? 1 : 0,aFonts[0].lf.lfItalic,aFonts[0].lf.lfUnderline, 2 * abs(aFonts[0].lf.lfHeight) * 74 / logPixelSY);
					Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", szStyle);
				} else {
					iii = lin->bIsHighlighted ? 16 : EventToIndex(lin);
					mir_snprintf(szStyle, SIZEOF(szStyle), "\\f0\\cf%u\\ul0\\highlight0\\b%d\\i%d\\ul%d\\fs%u", iii + 1, aFonts[0].lf.lfWeight >= FW_BOLD ? 1 : 0, aFonts[0].lf.lfItalic,aFonts[0].lf.lfUnderline ,2 * abs(aFonts[0].lf.lfHeight) * 74 / logPixelSY);
					Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", szStyle);
				}
			} else
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", Log_SetStyle(0, 0));
			// insert a TAB if necessary to put the timestamp in the right position
			if (g_Settings.dwIconFlags)
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tab ");

			//insert timestamp
			if (g_Settings.ShowTime) {
				TCHAR szTimeStamp[30], szOldTimeStamp[30];

				lstrcpyn(szTimeStamp, MakeTimeStamp(g_Settings.pszTimeStamp, lin->time), 30);
				lstrcpyn(szOldTimeStamp, MakeTimeStamp(g_Settings.pszTimeStamp, streamData->si->LastTime), 30);
				if (!g_Settings.ShowTimeIfChanged || streamData->si->LastTime == 0 || lstrcmp(szTimeStamp, szOldTimeStamp)) {
					streamData->si->LastTime = lin->time;
					Log_AppendRTF(streamData, TRUE, &buffer, &bufferEnd, &bufferAlloced, _T("%s"), szTimeStamp);
				}
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tab ");
			}

			// Insert the nick
			if (lin->ptszNick && lin->iType == GC_EVENT_MESSAGE) {
				TCHAR pszTemp[300], *p1;
				STATUSINFO *ti;
				char pszIndicator[3] = "\0\0";
				int  crNickIndex = 0;
													//mad
				if (g_Settings.LogClassicIndicators/*g_Settings.ClassicIndicators */||g_Settings.ColorizeNicksInLog) {
					USERINFO *ui = streamData->si->pUsers;
					while (ui) {
						if (!lstrcmp(ui->pszNick, lin->ptszNick)) {
							ti = TM_FindStatus(streamData->si->pStatuses, TM_WordToString(streamData->si->pStatuses, ui->Status));
							if (ti && (int)ti->hIcon < streamData->si->iStatusCount) {
								int id = streamData->si->iStatusCount - (int)ti->hIcon - 1;
								switch (id) {
									case 1:
										pszIndicator[0] = '+';
										crNickIndex = 2;
										break;
									case 2:
										pszIndicator[0] = '%';
										crNickIndex = 1;
										break;
									case 3:
										pszIndicator[0] = '@';
										crNickIndex = 0;
										break;
									case 4:
										pszIndicator[0] = '!';
										crNickIndex = 3;
										break;
									case 5:
										pszIndicator[0] = '*';
										crNickIndex = 4;
										break;
									default:
										pszIndicator[0] = 0;
								}
							}
							break;
						}
						ui = ui->next;
					}
				}

				Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", Log_SetStyle(lin->bIsMe ? 2 : 1, lin->bIsMe ? 2 : 1));
													//MAD
				if (g_Settings.LogClassicIndicators /*g_Settings.ClassicIndicators*/)
					Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s", pszIndicator);

				lstrcpyn(pszTemp, lin->bIsMe ? g_Settings.pszOutgoingNick : g_Settings.pszIncomingNick, 299);
				p1 = _tcsstr(pszTemp, _T("%n"));
				if (p1)
					p1[1] = 's';

				if (!lin->bIsMe) {
					if (g_Settings.ClickableNicks) {
						_tcsnrplc(pszTemp, 300, _T("%s"), _T("~~++#%s#++~~"));
						pszTemp[299] = 0;
					}
					//Log_Append(&buffer, &bufferEnd, &bufferAlloced, "~~++#");
					if (g_Settings.ColorizeNicksInLog && pszIndicator[0])
						Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\cf%u ", OPTIONS_FONTCOUNT + streamData->crCount + crNickIndex + 1);
				}

				Log_AppendRTF(streamData, TRUE, &buffer, &bufferEnd, &bufferAlloced, pszTemp, lin->ptszNick);
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, " ");
			}

			// Insert the message
			{
				i = lin->bIsHighlighted ? 16 : EventToIndex(lin);
				Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", Log_SetStyle(i, i));
				streamData->lin = lin;
				AddEventToBuffer(&buffer, &bufferEnd, &bufferAlloced, streamData);
			}
		}
		lin = lin->prev;
	}

	// ### RTF END
	if (streamData->bRedraw)
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\par}");
	else
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "}");
	return buffer;
}

static DWORD CALLBACK Log_StreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
	LOGSTREAMDATA *lstrdat = (LOGSTREAMDATA *) dwCookie;

	if (lstrdat) {
		// create the RTF
		if (lstrdat->buffer == NULL) {
			lstrdat->bufferOffset = 0;
			lstrdat->buffer = Log_CreateRTF(lstrdat);
			lstrdat->bufferLen = lstrlenA(lstrdat->buffer);
		}

		// give the RTF to the RE control
		*pcb = min(cb, lstrdat->bufferLen - lstrdat->bufferOffset);
		CopyMemory(pbBuff, lstrdat->buffer + lstrdat->bufferOffset, *pcb);
		lstrdat->bufferOffset += *pcb;

		// free stuff if the streaming operation is complete
		if (lstrdat->bufferOffset == lstrdat->bufferLen) {
			mir_free(lstrdat->buffer);
			lstrdat->buffer = NULL;
		}
	}

	return 0;
}

void Log_StreamInEvent(HWND hwndDlg,  LOGINFO* lin, SESSION_INFO* si, BOOL bRedraw, BOOL bPhaseTwo)
{
	EDITSTREAM stream;
	LOGSTREAMDATA streamData;
	CHARRANGE oldsel, sel, newsel;
	POINT point = {0};
	SCROLLINFO scroll;
	WPARAM wp;
	HWND hwndRich;
	struct _MessageWindowData *dat = (struct _MessageWindowData *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	if (hwndDlg == 0 || lin == 0 || si == 0 || dat == 0)
		return;

	hwndRich = GetDlgItem(hwndDlg, IDC_CHAT_LOG);
	ZeroMemory(&streamData, sizeof(LOGSTREAMDATA));
	streamData.hwnd = hwndRich;
	streamData.si = si;
	streamData.lin = lin;
	streamData.bStripFormat = FALSE;
	streamData.dat = dat;

	//	bPhaseTwo = bRedraw && bPhaseTwo;

	if (bRedraw || si->iType != GCW_CHATROOM || !si->bFilterEnabled || (si->iLogFilterFlags&lin->iType) != 0) {
		BOOL bFlag = FALSE, fDoReplace;

		ZeroMemory(&stream, sizeof(stream));
		stream.pfnCallback = Log_StreamCallback;
		stream.dwCookie = (DWORD_PTR) & streamData;
		scroll.cbSize = sizeof(SCROLLINFO);
		scroll.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
		GetScrollInfo(GetDlgItem(hwndDlg, IDC_CHAT_LOG), SB_VERT, &scroll);
		SendMessage(hwndRich, EM_GETSCROLLPOS, 0, (LPARAM) &point);

		// do not scroll to bottom if there is a selection
		SendMessage(hwndRich, EM_EXGETSEL, 0, (LPARAM) &oldsel);
		if (oldsel.cpMax != oldsel.cpMin)
			SendMessage(hwndRich, WM_SETREDRAW, FALSE, 0);

		//set the insertion point at the bottom
		sel.cpMin = sel.cpMax = GetRichTextLength(hwndRich);
		SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM) & sel);

		// fix for the indent... must be a M$ bug
		if (sel.cpMax == 0)
			bRedraw = TRUE;

		// should the event(s) be appended to the current log
		wp = bRedraw ? SF_RTF : SFF_SELECTION | SF_RTF;

		//get the number of pixels per logical inch
		if (bRedraw) {
			HDC hdc;
			hdc = GetDC(NULL);
			logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
			logPixelSX = GetDeviceCaps(hdc, LOGPIXELSX);
			ReleaseDC(NULL, hdc);
			SendMessage(hwndRich, WM_SETREDRAW, FALSE, 0);
			bFlag = TRUE;
			//			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}

		// stream in the event(s)
		streamData.lin = lin;
		streamData.bRedraw = bRedraw;
		SendMessage(hwndRich, EM_STREAMIN, wp, (LPARAM) & stream);


		//SendMessage(hwndRich, EM_EXGETSEL, (WPARAM)0, (LPARAM)&newsel);
		/*
		* for new added events, only replace in message or action events.
		* no need to replace smileys or math formulas elsewhere
		*/

		fDoReplace = (bRedraw || (lin->ptszText
								  && (lin->iType == GC_EVENT_MESSAGE || lin->iType == GC_EVENT_ACTION)));


		/*
		 * use mathmod to replace formulas
		 */
		if (g_Settings.MathMod && fDoReplace) {
			TMathRicheditInfo mathReplaceInfo;
			CHARRANGE mathNewSel;
			mathNewSel.cpMin = sel.cpMin;

			if (mathNewSel.cpMin < 0)
				mathNewSel.cpMin = 0;

			mathNewSel.cpMax = -1;

			mathReplaceInfo.hwndRichEditControl = hwndRich;

			if (!bRedraw)
				mathReplaceInfo.sel = &mathNewSel;
			else
				mathReplaceInfo.sel = 0;

			mathReplaceInfo.disableredraw = TRUE;
			CallService(MATH_RTF_REPLACE_FORMULAE, 0, (LPARAM)&mathReplaceInfo);
			bFlag = TRUE;
		}

		/*
		 * replace marked nicknames with hyperlinks to make the nicks
		 * clickable
		 */

		if (g_Settings.ClickableNicks) {
			CHARFORMAT2 cf2;
			FINDTEXTEX fi, fi2;

			ZeroMemory(&cf2, sizeof(CHARFORMAT2));
			fi2.lpstrText = _T("#++~~");
			fi.chrg.cpMin = bRedraw ? 0 : sel.cpMin;
			fi.chrg.cpMax = -1;
			fi.lpstrText = _T("~~++#");
			cf2.cbSize = sizeof(cf2);

			while (SendMessage(hwndRich, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&fi) > -1) {
				fi2.chrg.cpMin = fi.chrgText.cpMin;
				fi2.chrg.cpMax = -1;

				if (SendMessage(hwndRich, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&fi2) > -1) {

					SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM)&fi.chrgText);
					SendMessage(hwndRich, EM_REPLACESEL, TRUE, (LPARAM)_T(""));
					fi2.chrgText.cpMin -= fi.chrgText.cpMax - fi.chrgText.cpMin;
					fi2.chrgText.cpMax -= fi.chrgText.cpMax - fi.chrgText.cpMin;
					SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM)&fi2.chrgText);
					SendMessage(hwndRich, EM_REPLACESEL, TRUE, (LPARAM)_T(""));
					fi2.chrgText.cpMax = fi2.chrgText.cpMin;

					fi2.chrgText.cpMin = fi.chrgText.cpMin;
					SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM)&fi2.chrgText);
// 					if (g_Settings.ColorizeNicksInLog)
// 						{
						cf2.dwMask = CFM_PROTECTED;
						cf2.dwEffects = CFE_PROTECTED;
// 						}
// 					else
// 						{
// 						cf2.dwMask = CFM_LINK;
// 						cf2.dwEffects = CFE_LINK;
// 						}
//
					SendMessage(hwndRich, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
				}
				fi.chrg.cpMin = fi.chrgText.cpMax;
			}
			SendMessage(hwndRich, EM_SETSEL, -1, -1);
		}


		/*
		* run smileyadd
		*/
		if (_Plugin.g_SmileyAddAvail && fDoReplace) {
			SMADD_RICHEDIT3 sm = {0};

			newsel.cpMax = -1;
			newsel.cpMin = sel.cpMin;
			if (newsel.cpMin < 0)
				newsel.cpMin = 0;
			ZeroMemory(&sm, sizeof(sm));
			sm.cbSize = sizeof(sm);
			sm.hwndRichEditControl = hwndRich;
			sm.Protocolname = si->pszModule;
			sm.rangeToReplace = bRedraw ? NULL : &newsel;
			sm.disableRedraw = TRUE;
			sm.hContact = si->hContact;
			CallService(MS_SMILEYADD_REPLACESMILEYS, 0, (LPARAM)&sm);
			}

		/*
		* trim the message log to the number of most recent events
		* this uses hidden marks in the rich text to find the events which should be deleted
		*/


		if (si->wasTrimmed) {
			TCHAR szPattern[50];
			FINDTEXTEX fi;

			mir_sntprintf(szPattern, SIZEOF(szPattern), _T("~-+%d+-~"), si->pLogEnd);
			fi.lpstrText = szPattern;
			fi.chrg.cpMin = 0;
			fi.chrg.cpMax = -1;
			if (SendMessage(hwndRich, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&fi) != 0) {
				CHARRANGE sel;
				sel.cpMin = 0;
				sel.cpMax = 20;
				SendMessage(hwndRich, EM_SETSEL, 0, fi.chrgText.cpMax + 1);
				SendMessage(hwndRich, EM_REPLACESEL, TRUE, (LPARAM)_T(""));
			}
			si->wasTrimmed = FALSE;
		}

		// scroll log to bottom if the log was previously scrolled to bottom, else restore old position
		if (bRedraw || (UINT)scroll.nPos >= (UINT)scroll.nMax - scroll.nPage - 5 || scroll.nMax - scroll.nMin - scroll.nPage < 50)
			SendMessage(GetParent(hwndRich), GC_SCROLLTOBOTTOM, 0, 0);
		else
			SendMessage(hwndRich, EM_SETSCROLLPOS, 0, (LPARAM) &point);

		// do we need to restore the selection
		if (oldsel.cpMax != oldsel.cpMin) {
			SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM) & oldsel);
			SendMessage(hwndRich, WM_SETREDRAW, TRUE, 0);
			InvalidateRect(hwndRich, NULL, TRUE);
		}

		// need to invalidate the window
		if (bFlag) {
			sel.cpMin = sel.cpMax = GetRichTextLength(hwndRich);
			SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM) & sel);
			SendMessage(hwndRich, WM_SETREDRAW, TRUE, 0);
			InvalidateRect(hwndRich, NULL, TRUE);
		}
	}
}

char * Log_CreateRtfHeader(MODULEINFO * mi)
{
	char *buffer;
	int bufferAlloced, bufferEnd, i = 0;

	// guesstimate amount of memory for the RTF header
	bufferEnd = 0;
	bufferAlloced = 4096;
	buffer = (char *) mir_realloc(mi->pszHeader, bufferAlloced);
	buffer[0] = '\0';


	//get the number of pixels per logical inch
	if (logPixelSY == 0) {
		HDC hdc;
		hdc = GetDC(NULL);
		logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
		logPixelSX = GetDeviceCaps(hdc, LOGPIXELSX);
		ReleaseDC(NULL, hdc);
	}

	// ### RTF HEADER

	// font table
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "{\\rtf1\\ansi\\deff0{\\fonttbl");
	for (i = 0; i < OPTIONS_FONTCOUNT ; i++)
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "{\\f%u\\fnil\\fcharset%u" TCHAR_STR_PARAM ";}", i, aFonts[i].lf.lfCharSet, aFonts[i].lf.lfFaceName);

	// colour table
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "}{\\colortbl ;");

	for (i = 0; i < OPTIONS_FONTCOUNT; i++)
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(aFonts[i].color), GetGValue(aFonts[i].color), GetBValue(aFonts[i].color));

	for (i = 0; i < mi->nColorCount; i++)
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(mi->crColors[i]), GetGValue(mi->crColors[i]), GetBValue(mi->crColors[i]));

	for (i = 0; i < 5; i++)
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(g_Settings.nickColors[i]), GetGValue(g_Settings.nickColors[i]), GetBValue(g_Settings.nickColors[i]));

	// new paragraph
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "}\\pard\\sl%d", 1000);

	// set tabs and indents
	{
		int iIndent = 0;

		if (g_Settings.LogSymbols) {
			TCHAR szString[2];
			LOGFONT lf;
			HFONT hFont;
			int iText;

			szString[1] = 0;
			szString[0] = 0x28;
			LoadMsgDlgFont(FONTSECTION_CHAT, 17, &lf, NULL, CHAT_FONTMODULE);
			hFont = CreateFontIndirect(&lf);
			iText = GetTextPixelSize(szString, hFont, TRUE) + 3;
			DeleteObject(hFont);
			iIndent += (iText * 1440) / logPixelSX;
			Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tx%u", iIndent);
		} else if (g_Settings.dwIconFlags) {
			iIndent += ((g_Settings.ScaleIcons ? 14 : 20) * 1440) / logPixelSX;
			Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tx%u", iIndent);
		}
		if (g_Settings.ShowTime) {
			int iSize = (g_Settings.LogTextIndent * 1440) / logPixelSX;
			Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tx%u", iIndent + iSize);
			if (g_Settings.LogIndentEnabled)
				iIndent += iSize;
		}
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\fi-%u\\li%u", iIndent, iIndent);
	}
	return buffer;
}

#define RTFPICTHEADERMAXSIZE   78
void LoadMsgLogBitmaps(void)
{
	HICON hIcon;
	HBITMAP hBmp, hoBmp;
	HDC hdc, hdcMem;
	BITMAPINFOHEADER bih = { 0 };
	int widthBytes, i;
	RECT rc;
	HBRUSH hBkgBrush;
	int rtfHeaderSize;
	PBYTE pBmpBits;
	int iIconSize;
	int sizeX = 0, sizeY = 0;

	if (hIcons[0])
		GetIconSize(hIcons[0], &sizeX, &sizeY);
	else
		sizeX = 16;

	if (sizeX >= 12)
		iIconSize = g_Settings.ScaleIcons ? 12 : 16;
	else
		iIconSize = sizeX;

	hBkgBrush = CreateSolidBrush(M->GetDword("Chat", "ColorLogBG", SRMSGDEFSET_BKGCOLOUR));
	bih.biSize = sizeof(bih);
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	bih.biHeight = iIconSize;
	bih.biPlanes = 1;
	bih.biWidth = iIconSize;
	widthBytes = ((bih.biWidth * bih.biBitCount + 31) >> 5) * 4;
	rc.top = rc.left = 0;
	rc.right = iIconSize;
	rc.bottom = iIconSize;
	hdc = GetDC(NULL);
	hBmp = CreateCompatibleBitmap(hdc, bih.biWidth, bih.biHeight);
	hdcMem = CreateCompatibleDC(hdc);
	pBmpBits = (PBYTE) mir_alloc(widthBytes * bih.biHeight);
	for (i = 0; i < SIZEOF(pLogIconBmpBits); i++) {
		hIcon = hIcons[i];
		pLogIconBmpBits[i] = (PBYTE) mir_alloc(RTFPICTHEADERMAXSIZE + (bih.biSize + widthBytes * bih.biHeight) * 2);
		rtfHeaderSize = sprintf((char *)pLogIconBmpBits[i], "{\\pict\\dibitmap0\\wbmbitspixel%u\\wbmplanes1\\wbmwidthbytes%u\\picw%u\\pich%u ", bih.biBitCount, widthBytes, bih.biWidth, bih.biHeight);
		hoBmp = (HBITMAP) SelectObject(hdcMem, hBmp);
		FillRect(hdcMem, &rc, hBkgBrush);
		DrawIconEx(hdcMem, 0, 1, hIcon, iIconSize, iIconSize, 0, NULL, DI_NORMAL);

		SelectObject(hdcMem, hoBmp);
		GetDIBits(hdc, hBmp, 0, bih.biHeight, pBmpBits, (BITMAPINFO *) & bih, DIB_RGB_COLORS);
		{
			int n;
			for (n = 0; n < sizeof(BITMAPINFOHEADER); n++)
				sprintf((char *)pLogIconBmpBits[i] + rtfHeaderSize + n * 2, "%02X", ((PBYTE) & bih)[n]);
			for (n = 0; n < widthBytes * bih.biHeight; n += 4)
				sprintf((char *)pLogIconBmpBits[i] + rtfHeaderSize + (bih.biSize + n) * 2, "%02X%02X%02X%02X", pBmpBits[n], pBmpBits[n + 1], pBmpBits[n + 2], pBmpBits[n + 3]);
		}
		logIconBmpSize[i] = rtfHeaderSize + (bih.biSize + widthBytes * bih.biHeight) * 2 + 1;
		pLogIconBmpBits[i][logIconBmpSize[i] - 1] = '}';
	}
	mir_free(pBmpBits);
	DeleteDC(hdcMem);
	DeleteObject(hBmp);
	ReleaseDC(NULL, hdc);
	DeleteObject(hBkgBrush);

	/* cache RTF font headers */

	//get the number of pixels per logical inch
	if (logPixelSY == 0) {
		HDC hdc;
		hdc = GetDC(NULL);
		logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
		logPixelSX = GetDeviceCaps(hdc, LOGPIXELSX);
		ReleaseDC(NULL, hdc);
	}

	for (i = 0; i < OPTIONS_FONTCOUNT; i++)
		mir_snprintf(CHAT_rtfFontsGlobal[i], RTFCACHELINESIZE, "\\f%u\\cf%u\\ul0\\highlight0\\b%d\\i%d\\ul%d\\fs%u", i, i + 1, aFonts[i].lf.lfWeight >= FW_BOLD ? 1 : 0, aFonts[i].lf.lfItalic,aFonts[i].lf.lfUnderline ,2 * abs(aFonts[i].lf.lfHeight) * 74 / logPixelSY);
	CHAT_rtffonts = &(CHAT_rtfFontsGlobal[0][0]);
}

void FreeMsgLogBitmaps(void)
{
	int i;
	for (i = 0; i < SIZEOF(pLogIconBmpBits); i++)
		mir_free(pLogIconBmpBits[i]);
}
