/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

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
//#include "m_MathModule.h"

extern      void ReleaseRichEditOle(IRichEditOle *ole);
extern      MYGLOBALS myGlobals;
extern      struct RTFColorTable *rtf_ctable;
extern      void ImageDataInsertBitmap(IRichEditOle *ole, HBITMAP hBm);


struct CPTABLE cpTable[] = {
	{ 874,	_T("Thai")	 },
	{ 932,	_T("Japanese") },
	{ 936,	_T("Simplified Chinese") },
	{ 949,	_T("Korean") },
	{ 950,	_T("Traditional Chinese") },
	{ 1250,	_T("Central European") },
	{ 1251,	_T("Cyrillic") },
	{ 20866,  _T("Cyrillic KOI8-R") },
	{ 1252,	_T("Latin I") },
	{ 1253,	_T("Greek") },
	{ 1254,	_T("Turkish") },
	{ 1255,	_T("Hebrew") },
	{ 1256,	_T("Arabic") },
	{ 1257,	_T("Baltic") },
	{ 1258,	_T("Vietnamese") },
	{ 1361,	_T("Korean (Johab)") },
	{   -1,     NULL}
};

static TCHAR    *Template_MakeRelativeDate(struct MessageWindowData *dat, time_t check, int groupBreak, TCHAR code);
static void     ReplaceIcons(HWND hwndDlg, struct MessageWindowData *dat, LONG startAt, int fAppend, BOOL isSent);

static TCHAR *weekDays[] = {_T("Sunday"), _T("Monday"), _T("Tuesday"), _T("Wednesday"), _T("Thursday"), _T("Friday"), _T("Saturday")};
static TCHAR *months[] = {_T("January"), _T("February"), _T("March"), _T("April"), _T("May"), _T("June"), _T("July"), _T("August"), _T("September"), _T("October"), _T("November"), _T("December")};

static TCHAR weekDays_translated[7][30];
static TCHAR months_translated[12][30];

static time_t today;

int g_groupBreak = TRUE;
static TCHAR *szMyName = NULL;
static TCHAR *szYourName = NULL;

extern TCHAR *FormatRaw(DWORD dwFlags, const TCHAR *msg, int flags, const char *szProto, HANDLE hContact, BOOL *clr_added, BOOL isSent);

static int logPixelSY;
static TCHAR szToday[22], szYesterday[22];
char rtfFontsGlobal[MSGDLGFONTCOUNT + 2][RTFCACHELINESIZE];
char *rtfFonts;

LOGFONTA logfonts[MSGDLGFONTCOUNT + 2];
COLORREF fontcolors[MSGDLGFONTCOUNT + 2];

#define LOGICON_MSG  0
#define LOGICON_URL  1
#define LOGICON_FILE 2
#define LOGICON_OUT 3
#define LOGICON_IN 4
#define LOGICON_STATUS 5
#define LOGICON_ERROR 6

static HICON Logicons[NR_LOGICONS];

#define STREAMSTAGE_HEADER  0
#define STREAMSTAGE_EVENTS  1
#define STREAMSTAGE_TAIL    2
#define STREAMSTAGE_STOP    3
struct LogStreamData {
	int stage;
	HANDLE hContact;
	HANDLE hDbEvent, hDbEventLast;
	char *buffer;
	int bufferOffset, bufferLen;
	int eventsToInsert;
	int isEmpty;
	int isAppend;
	struct MessageWindowData *dlgDat;
	DBEVENTINFO *dbei;
};

__forceinline char *GetRTFFont(DWORD dwIndex)
{
	return rtfFonts + (dwIndex * RTFCACHELINESIZE);
}

int safe_wcslen(wchar_t *msg, int chars)
{
	int i;

	for (i = 0; i < chars; i++) {
		if (msg[i] == (WCHAR)0)
			return i;
	}
	return 0;
}
/*
 * remove any empty line at the end of a message to avoid some RichEdit "issues" with
 * the highlight code (individual background colors).
 * Doesn't touch the message for sure, but empty lines at the end are ugly anyway.
 */

static void TrimMessage(TCHAR *msg)
{
	size_t iLen = _tcslen(msg) - 1;
	size_t i = iLen;

	while (i && (msg[i] == '\r' || msg[i] == '\n')) {
		i--;
	}
	if (i < iLen)
		msg[i+1] = '\0';
}

void CacheLogFonts()
{
	int i;
	HDC hdc = GetDC(NULL);
	logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
	ReleaseDC(NULL, hdc);

	ZeroMemory((void *)logfonts, sizeof(LOGFONTA) * MSGDLGFONTCOUNT + 2);
	for (i = 0; i < MSGDLGFONTCOUNT; i++) {
		LoadLogfont(i, &logfonts[i], &fontcolors[i], FONTMODULE);
		wsprintfA(rtfFontsGlobal[i], "\\f%u\\cf%u\\b%d\\i%d\\ul%d\\fs%u", i, i, logfonts[i].lfWeight >= FW_BOLD ? 1 : 0, logfonts[i].lfItalic,logfonts[i].lfUnderline, 2 * abs(logfonts[i].lfHeight) * 74 / logPixelSY);
	}
	wsprintfA(rtfFontsGlobal[MSGDLGFONTCOUNT], "\\f%u\\cf%u\\b%d\\i%d\\fs%u", MSGDLGFONTCOUNT, MSGDLGFONTCOUNT, 0, 0, 0);

	_tcsncpy(szToday, TranslateT("Today"), 20);
	_tcsncpy(szYesterday, TranslateT("Yesterday"), 20);
	szToday[19] = szYesterday[19] = 0;

	/*
	 * cache/create the info panel fonts
	 */

	myGlobals.ipConfig.isValid = 1;

	if (myGlobals.ipConfig.isValid) {
		COLORREF clr;
		LOGFONTA lf;

		for (i = 0; i < IPFONTCOUNT; i++) {
			if (myGlobals.ipConfig.hFonts[i])
				DeleteObject(myGlobals.ipConfig.hFonts[i]);
			LoadLogfont(i + 100, &lf, &clr, FONTMODULE);
			lf.lfHeight =-MulDiv(lf.lfHeight, logPixelSY, 72);
			myGlobals.ipConfig.hFonts[i] = CreateFontIndirectA(&lf);
			myGlobals.ipConfig.clrs[i] = clr;
		}
		myGlobals.hFontCaption = myGlobals.ipConfig.hFonts[IPFONTCOUNT - 1];
	}

	if (myGlobals.ipConfig.bkgBrush)
		DeleteObject(myGlobals.ipConfig.bkgBrush);

	myGlobals.ipConfig.clrBackground = DBGetContactSettingDword(NULL, FONTMODULE, "ipfieldsbg", GetSysColor(COLOR_3DFACE));
	myGlobals.ipConfig.clrClockSymbol = DBGetContactSettingDword(NULL, FONTMODULE, "col_clock", GetSysColor(COLOR_WINDOWTEXT));

	myGlobals.ipConfig.bkgBrush = CreateSolidBrush(myGlobals.ipConfig.clrBackground);

	myGlobals.crDefault = DBGetContactSettingDword(NULL, FONTMODULE, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR);
	myGlobals.crIncoming = DBGetContactSettingDword(NULL, FONTMODULE, "inbg", SRMSGDEFSET_BKGINCOLOUR);
	myGlobals.crOutgoing = DBGetContactSettingDword(NULL, FONTMODULE, "outbg", SRMSGDEFSET_BKGOUTCOLOUR);
	myGlobals.crStatus = DBGetContactSettingDword(NULL, FONTMODULE, "statbg", SRMSGDEFSET_BKGCOLOUR);
	myGlobals.crOldIncoming =DBGetContactSettingDword(NULL, FONTMODULE, "oldinbg", SRMSGDEFSET_BKGINCOLOUR);
	myGlobals.crOldOutgoing =DBGetContactSettingDword(NULL, FONTMODULE, "oldoutbg", SRMSGDEFSET_BKGOUTCOLOUR);
}

void FreeLogFonts()
{
	int i;

	for (i = 0; i < IPFONTCOUNT; i++)
		if (myGlobals.ipConfig.hFonts[i])
			DeleteObject(myGlobals.ipConfig.hFonts[i]);

	if (myGlobals.ipConfig.bkgBrush)
		DeleteObject(myGlobals.ipConfig.bkgBrush);
}

/*
#define RTFPICTHEADERMAXSIZE   78
*/

/*
 * cache copies of all our msg log icons with 3 background colors to speed up the
 * process of inserting icons into the RichEdit control.
 */

void CacheMsgLogIcons()
{
	Logicons[0] = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
	Logicons[1] = LoadSkinnedIcon(SKINICON_EVENT_URL);
	Logicons[2] = LoadSkinnedIcon(SKINICON_EVENT_FILE);
	Logicons[3] = myGlobals.g_iconOut;
	Logicons[4] = myGlobals.g_iconIn;
	Logicons[5] = myGlobals.g_iconStatus;
	Logicons[6] = myGlobals.g_iconErr;
}

/*
 * pre-translate day and month names to significantly reduce he number of Translate()
 * service calls while building the message log
 */

void PreTranslateDates()
{
	int i;
	TCHAR *szTemp;

	for (i = 0; i <= 6; i++) {
		szTemp = TranslateTS(weekDays[i]);
		mir_sntprintf(weekDays_translated[i], 28, _T("%s"), szTemp);
	}
	for (i = 0; i <= 11; i++) {
		szTemp = TranslateTS(months[i]);
		mir_sntprintf(months_translated[i], 28, _T("%s"), szTemp);
	}
}

static int GetColorIndex(char *rtffont)
{
	char *p;

	if ((p = strstr(rtffont, "\\cf")) != NULL)
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

	int  lineLen = (int)(wcslen(line)) * 9 + 8;
	if (*cbBufferEnd + lineLen > *cbBufferAlloced) {
		cbBufferAlloced[0] += (lineLen + 1024UL - lineLen % 1024UL);
		*buffer = (char *) realloc(*buffer, *cbBufferAlloced);
	}

	d = *buffer + *cbBufferEnd;
	strcpy(d, "{\\uc1 ");
	d += 6;

	for (; *line; line++, textCharsCount++) {

		if (1) {
			if (*line == 127 && line[1] != 0) {
				TCHAR code = line[2];
				if (((code == '0' || code == '1') && line[3] == ' ') || (line[1] == 'c' && code == 'x')) {
					int begin = (code == '1');
					switch (line[1]) {
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
						case 's':
							CopyMemory(d, begin ? "\\strike " : "\\strike0 ", begin ? 8 : 9);
							d += (begin ? 8 : 9);
							line += 3;
							continue;
						case 'c':
							begin = (code == 'x');
							CopyMemory(d, "\\cf", 3);
							if (begin) {
								d[3] = (char)line[3];
								d[4] = (char)line[4];
								d[5] = ' ';
							} else {
								char szTemp[10];
								int colindex = GetColorIndex(GetRTFFont(LOWORD(mode) ? (MSGFONTID_MYMSG + (HIWORD(mode) ? 8 : 0)) : (MSGFONTID_YOURMSG + (HIWORD(mode) ? 8 : 0))));
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
			CopyMemory(d, "\\line ", 6);
			line++;
			d += 6;
		} else if (*line == '\n') {
			CopyMemory(d, "\\line ", 6);
			d += 6;
		} else if (*line == '\t') {
			CopyMemory(d, "\\tab ", 5);
			d += 5;
		} else if (*line == '\\' || *line == '{' || *line == '}') {
			*d++ = '\\';
			*d++ = (char) * line;
		} else if (*line < 128) {
			*d++ = (char) * line;
		} else
			d += sprintf(d, "\\u%d ?", *line);
	}

	strcpy(d, "}");
	d++;

	*cbBufferEnd = (int)(d - *buffer);
	return textCharsCount;
}
#endif

/*
 * same as above but does "\r\n"->"\\par " and "\t"->"\\tab " too
 */

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

		if (1) {
			if ((*buffer)[i] == '' && (*buffer)[i + 1] != 0) {
				char code = (*buffer)[i + 2];
				char tag = (*buffer)[i + 1];

				if (((code == '0' || code == '1') && (*buffer)[i + 3] == ' ') || (tag == 'c' && (code == 'x' || code == '0'))) {
					int begin = (code == '1');

					if (*cbBufferEnd + 5 > *cbBufferAlloced) {
						*cbBufferAlloced += 1024;
						*buffer = (char *) realloc(*buffer, *cbBufferAlloced);
					}
					switch (tag) {
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
						case 's':
							*cbBufferAlloced += 20;
							*buffer = (char *)realloc(*buffer, *cbBufferAlloced);
							MoveMemory(*buffer + i + 6, *buffer + i + 1, (*cbBufferEnd - i) + 1);
							CopyMemory(*buffer + i, begin ? "\\strike1 " : "\\strike0 ", begin ? 9 : 9);
							*cbBufferEnd += 5;
							continue;
						case 'c':
							begin = (code == 'x');
							CopyMemory(*buffer + i, "\\cf", 3);
							if (begin) {
							} else {
								char szTemp[10];
								int colindex = GetColorIndex(GetRTFFont(LOWORD(mode) ? (MSGFONTID_MYMSG + (HIWORD(mode) ? 8 : 0)) : (MSGFONTID_YOURMSG + (HIWORD(mode) ? 8 : 0))));
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
			MoveMemory(*buffer + i + 6, *buffer + i + 2, *cbBufferEnd - i - 1);
			CopyMemory(*buffer + i, "\\line ", 6);
			*cbBufferEnd += 4;
		} else if ((*buffer)[i] == '\n') {
			if (*cbBufferEnd + 6 > *cbBufferAlloced) {
				*cbBufferAlloced += 1024;
				*buffer = (char *) realloc(*buffer, *cbBufferAlloced);
			}
			MoveMemory(*buffer + i + 6, *buffer + i + 1, *cbBufferEnd - i);
			CopyMemory(*buffer + i, "\\line ", 6);
			*cbBufferEnd += 5;
		} else if ((*buffer)[i] == '\t') {
			if (*cbBufferEnd + 5 > *cbBufferAlloced) {
				*cbBufferAlloced += 1024;
				*buffer = (char *) realloc(*buffer, *cbBufferAlloced);
			}
			MoveMemory(*buffer + i + 5, *buffer + i + 1, *cbBufferEnd - i);
			CopyMemory(*buffer + i, "\\tab ", 5);
			*cbBufferEnd += 4;
		} else if ((*buffer)[i] == '\\' || (*buffer)[i] == '{' || (*buffer)[i] == '}') {
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
	return (int)(_mbslen(*buffer + *cbBufferEnd));
}

static void Build_RTF_Header(char **buffer, int *bufferEnd, int *bufferAlloced, struct MessageWindowData *dat)
{
	COLORREF colour;
	int      i;
	char     szTemp[30];

	// rtl
	if (dat->dwFlags & MWF_LOG_RTL)
		AppendToBuffer(buffer, bufferEnd, bufferAlloced, "{\\rtf1\\ansi\\deff0{\\fonttbl");
	else
		AppendToBuffer(buffer, bufferEnd, bufferAlloced, "{\\rtf1\\ansi\\deff0{\\fonttbl");

	for (i = 0; i < MSGDLGFONTCOUNT; i++)
		AppendToBuffer(buffer, bufferEnd, bufferAlloced, "{\\f%u\\fnil\\fcharset%u %s;}", i, dat->theme.logFonts[i].lfCharSet, dat->theme.logFonts[i].lfFaceName);
	AppendToBuffer(buffer, bufferEnd, bufferAlloced, "{\\f%u\\fnil\\fcharset%u %s;}", MSGDLGFONTCOUNT, dat->theme.logFonts[i].lfCharSet, "Arial");

	AppendToBuffer(buffer, bufferEnd, bufferAlloced, "}{\\colortbl ");
	for (i = 0; i < MSGDLGFONTCOUNT; i++)
		AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(dat->theme.fontColors[i]), GetGValue(dat->theme.fontColors[i]), GetBValue(dat->theme.fontColors[i]));
	if (GetSysColorBrush(COLOR_HOTLIGHT) == NULL)
		colour = RGB(0, 0, 255);
	else
		colour = GetSysColor(COLOR_HOTLIGHT);
	AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));

	/* OnO: Create incoming and outcoming colours */
	colour = dat->theme.inbg;
	AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
	colour = dat->theme.outbg;
	AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
	colour = dat->theme.bg;
	AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
	colour = dat->theme.hgrid;
	AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
	colour = dat->theme.oldinbg;
	AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
	colour = dat->theme.oldoutbg;
	AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
	colour = dat->theme.statbg;
	AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));

	// custom template colors...

	for (i = 1; i <= 5; i++) {
		_snprintf(szTemp, 10, "cc%d", i);
		colour = dat->theme.custom_colors[i - 1];
		if (colour == 0)
			colour = RGB(1, 1, 1);
		AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(colour), GetGValue(colour), GetBValue(colour));
	}

	// bbcode colors...

	for (i = 0; i < myGlobals.rtf_ctablesize; i++)
		AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(rtf_ctable[i].clr), GetGValue(rtf_ctable[i].clr), GetBValue(rtf_ctable[i].clr));

	/*
	 * paragraph header
	 */
	AppendToBuffer(buffer, bufferEnd, bufferAlloced, "}");

	/*
	 * indent:
	 * real indent is set in msgdialog.c (DM_OPTIONSAPPLIED)
	 */

	if (!(dat->dwFlags & MWF_LOG_INDENT))
		AppendToBuffer(buffer, bufferEnd, bufferAlloced, "\\li%u\\ri%u\\fi%u\\tx%u", 2*15, 2*15, 0, 70 * 15);
}


//free() the return value
static char *CreateRTFHeader(struct MessageWindowData *dat)
{
	char *buffer;
	int bufferAlloced, bufferEnd;

	bufferEnd = 0;
	bufferAlloced = 1024;
	buffer = (char *) malloc(bufferAlloced);
	buffer[0] = '\0';

	Build_RTF_Header(&buffer, &bufferEnd, &bufferAlloced, dat);
	return buffer;
}

static void AppendTimeStamp(TCHAR *szFinalTimestamp, int isSent, char **buffer, int *bufferEnd, int *bufferAlloced, int skipFont,
							struct MessageWindowData *dat, int iFontIDOffset)
{
#ifdef _UNICODE
	if (skipFont)
		AppendUnicodeToBuffer(buffer, bufferEnd, bufferAlloced, szFinalTimestamp, MAKELONG(isSent, dat->isHistory));
	else {
		AppendToBuffer(buffer, bufferEnd, bufferAlloced, "%s ", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset));
		AppendUnicodeToBuffer(buffer, bufferEnd, bufferAlloced, szFinalTimestamp, MAKELONG(isSent, dat->isHistory));
	}
#else
	if (skipFont)
		AppendToBuffer(buffer, bufferEnd, bufferAlloced, "%s", szFinalTimestamp);
	else
		AppendToBuffer(buffer, bufferEnd, bufferAlloced, "%s %s", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset), szFinalTimestamp);
#endif
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
	int heFlags;

	switch (dbei->eventType) {
		case EVENTTYPE_MESSAGE:
			//case EVENTTYPE_STATUSCHANGE:
			return 1;
		case EVENTTYPE_URL:
			return (dat->dwFlagsEx & MWF_SHOW_URLEVENTS);
		case EVENTTYPE_FILE:
			return(dat->dwFlagsEx & MWF_SHOW_FILEEVENTS);
	}
	if (IsStatusEvent(dbei->eventType))
		return 1;

	heFlags = HistoryEvents_GetFlags(dbei->eventType);
	if (heFlags != -1)
		return (heFlags & HISTORYEVENTS_FLAG_SHOW_IM_SRMM) == HISTORYEVENTS_FLAG_SHOW_IM_SRMM;

	return 0;
}

static char *Template_CreateRTFFromDbEvent(struct MessageWindowData *dat, HANDLE hContact, HANDLE hDbEvent, int prefixParaBreak, struct LogStreamData *streamData)
{
	char *buffer, c;
	TCHAR ci, cc;
#if !defined(_UNICODE)
	char *szName;
#endif
	TCHAR 	*szFinalTimestamp;
	int 	bufferAlloced, bufferEnd;
	size_t 	iTemplateLen, i = 0;
	DBEVENTINFO dbei = { 0 };
	int isSent = 0;
	int iFontIDOffset = 0;
	TCHAR *szTemplate;
	time_t final_time;
	BOOL skipToNext = FALSE, showTime = TRUE, showDate = TRUE, skipFont = FALSE;
	struct tm event_time;
	TemplateSet *this_templateset;
	BOOL isBold = FALSE, isItalic = FALSE, isUnderline = FALSE;
	DWORD dwEffectiveFlags;
	DWORD dwFormattingParams = MAKELONG(myGlobals.m_FormatWholeWordsOnly, 0);
	char  *szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
	BOOL  fIsStatusChangeEvent = FALSE;
	TCHAR *msg, *formatted = NULL;
	int heFlags = -1;
	char *rtfMessage = NULL;

	bufferEnd = 0;
	bufferAlloced = 1024;
	buffer = (char *) malloc(bufferAlloced);
	buffer[0] = '\0';

	if (streamData->dbei != 0)
		dbei = *(streamData->dbei);
	else {
		dbei.cbSize = sizeof(dbei);
		dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM) hDbEvent, 0);
		if (dbei.cbBlob == -1) {
			free(buffer);
			return NULL;
		}
		dbei.pBlob = (PBYTE) malloc(dbei.cbBlob);
		CallService(MS_DB_EVENT_GET, (WPARAM) hDbEvent, (LPARAM) & dbei);
		if (!DbEventIsShown(dat, &dbei)) {
			free(dbei.pBlob);
			free(buffer);
			return NULL;
		}
	}

	if (dbei.eventType != EVENTTYPE_MESSAGE && dbei.eventType != EVENTTYPE_FILE
			&& dbei.eventType != EVENTTYPE_URL && !IsStatusEvent(dbei.eventType))
		heFlags = HistoryEvents_GetFlags(dbei.eventType);
	if (heFlags & HISTORYEVENTS_FLAG_DEFAULT)
		heFlags = -1;

	if (dbei.eventType == EVENTTYPE_MESSAGE && !isSent)
		dat->stats.lastReceivedChars = lstrlenA((char *) dbei.pBlob);

	if (heFlags != -1)
		rtfMessage = HistoryEvents_GetRichText(hDbEvent, &dbei);
	if (rtfMessage == NULL) {
		msg = DbGetEventTextT(&dbei, dat->codePage);
		if (!msg) {
			free(dbei.pBlob);
			free(buffer);
			return NULL;
		}
		TrimMessage(msg);
		formatted = FormatRaw(dat->dwFlags, msg, dwFormattingParams, szProto, dat->hContact, &dat->clr_added, isSent);
		mir_free(msg);
	}
	/*
	else
	{
#if defined( _UNICODE )
		int wlen;
		int msglen = lstrlenA((char *) dbei.pBlob) + 1;

		if ((dbei.cbBlob >= (DWORD)(2 * msglen)) && !(dat->sendMode & SMODE_FORCEANSI)) {
			msg = (wchar_t *) & dbei.pBlob[msglen];
			wlen = safe_wcslen(msg, (dbei.cbBlob - msglen) / 2);
			if (wlen <= (msglen - 1) && wlen > 0) {
				TrimMessage(msg);
				formatted = FormatRaw(dat->dwFlags, msg, dwFormattingParams, szProto, dat->hContact, &dat->clr_added, isSent);
			} else
				goto nounicode;
		} else {
	nounicode:
			msg = (TCHAR *) alloca(sizeof(TCHAR) * msglen);
			MultiByteToWideChar(dat->codePage, 0, (char *) dbei.pBlob, -1, msg, msglen);
			TrimMessage(msg);
			formatted = FormatRaw(dat->dwFlags, msg, dwFormattingParams, szProto, dat->hContact, &dat->clr_added, isSent);
		}
#else   // non-unicode
		msg = (char *) dbei.pBlob;
		TrimMessage(msg);
		formatted = FormatRaw(dat->dwFlags, msg, dwFormattingParams, szProto, dat->hContact, &dat->clr_added, isSent);
#endif
	}
	*/

	dat->stats.lastReceivedChars = 0;
	fIsStatusChangeEvent = (heFlags != -1 || IsStatusEvent(dbei.eventType));

	if (dat->isAutoRTL & 2) {                                     // means: last \\par was deleted to avoid new line at end of log
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\par");
		dat->isAutoRTL &= ~2;
	}

	if (dat->dwFlags & MWF_LOG_RTL)
		dbei.flags |= DBEF_RTL;

	if (dbei.flags & DBEF_RTL)
		dat->isAutoRTL |= 1;

	dwEffectiveFlags = dat->dwFlags;

	dat->isHistory = (dbei.timestamp < (DWORD)dat->stats.started && (dbei.flags & DBEF_READ || dbei.flags & DBEF_SENT));
	iFontIDOffset = dat->isHistory ? 8 : 0;     // offset into the font table for either history (old) or new events... (# of fonts per configuration set)
	isSent = (dbei.flags & DBEF_SENT);

	if (!isSent && (fIsStatusChangeEvent || dbei.eventType == EVENTTYPE_MESSAGE || dbei.eventType == EVENTTYPE_URL)) {
		 //MAD: ugly hack for hideOnClose...
		if(IsWindowVisible(GetParent(dat->hwnd))){
			CallService(MS_DB_EVENT_MARKREAD, (WPARAM)hContact, (LPARAM)hDbEvent);
			CallService(MS_CLIST_REMOVEEVENT, (WPARAM)hContact, (LPARAM)hDbEvent);
			}
	}

	g_groupBreak = TRUE;

	if (dwEffectiveFlags & MWF_DIVIDERWANTED) {
		static char szStyle_div[128] = "\0";
		if (szStyle_div[0] == 0)
			mir_snprintf(szStyle_div, 128, "\\f%u\\cf%u\\ul0\\b%d\\i%d\\fs%u", H_MSGFONTID_DIVIDERS, H_MSGFONTID_DIVIDERS, 0, 0, 5);

		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\sl-1\\slmult0\\highlight%d\\cf%d\\-\\par\\sl0", H_MSGFONTID_DIVIDERS, H_MSGFONTID_DIVIDERS);
		dat->dwFlags &= ~MWF_DIVIDERWANTED;
	}
	if (dwEffectiveFlags & MWF_LOG_GROUPMODE && ((dbei.flags & (DBEF_SENT | DBEF_READ | DBEF_RTL)) == LOWORD(dat->iLastEventType)) && dbei.eventType == EVENTTYPE_MESSAGE && HIWORD(dat->iLastEventType) == EVENTTYPE_MESSAGE && (dbei.timestamp - dat->lastEventTime) < 86400) {
		g_groupBreak = FALSE;
		if ((time_t)dbei.timestamp > today && dat->lastEventTime < today) {
			g_groupBreak = TRUE;
		}
	}
	if (!streamData->isEmpty && g_groupBreak && (dwEffectiveFlags & MWF_LOG_GRID))
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\sl-1\\slmult0\\highlight%d\\cf%d\\-\\par\\sl0", MSGDLGFONTCOUNT + 4, MSGDLGFONTCOUNT + 4);

	if (dbei.flags & DBEF_RTL)
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\rtlpar");
	else
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\ltrpar");

	/* OnO: highlight start */
	if (dwEffectiveFlags & MWF_LOG_INDIVIDUALBKG){
		if(fIsStatusChangeEvent)
			AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight%d\\cf%d", MSGDLGFONTCOUNT + 7, MSGDLGFONTCOUNT + 7);
		else
			AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight%d\\cf%d", MSGDLGFONTCOUNT + (dat->isHistory?5:1) + ((isSent) ? 1 : 0), MSGDLGFONTCOUNT + (dat->isHistory?5:1) + ((isSent) ? 1 : 0));
	}else if (dwEffectiveFlags & MWF_LOG_GRID)
 		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight%d\\cf%d", MSGDLGFONTCOUNT + 3, MSGDLGFONTCOUNT + 3);
 	else
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight%d\\cf%d", MSGDLGFONTCOUNT + 3, MSGDLGFONTCOUNT + 3);

	streamData->isEmpty = FALSE;

	if (dat->isAutoRTL & 1) {
		if (dbei.flags & DBEF_RTL) {
			AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\ltrch\\rtlch");
		} else {
			AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\rtlch\\ltrch");
		}
	}

	/*
	 * templated code starts here
	 */
	if (dwEffectiveFlags & MWF_LOG_SHOWTIME) {
		final_time = dbei.timestamp;
		if (dat->dwFlags & MWF_LOG_LOCALTIME) {
			if (!isSent && dat->timediff != 0)
				final_time = dbei.timestamp - dat->timediff;
		}
		_tzset();
		event_time = *localtime(&final_time);
	}
	this_templateset = dbei.flags & DBEF_RTL ? dat->rtl_templates : dat->ltr_templates;

	if (fIsStatusChangeEvent)
		szTemplate = this_templateset->szTemplates[TMPL_STATUSCHG];
	else if (dbei.eventType == EVENTTYPE_ERRMSG)
		szTemplate = this_templateset->szTemplates[TMPL_ERRMSG];
	else {
		if (dwEffectiveFlags & MWF_LOG_GROUPMODE)
			szTemplate = isSent ? (g_groupBreak ? this_templateset->szTemplates[TMPL_GRPSTARTOUT] : this_templateset->szTemplates[TMPL_GRPINNEROUT]) :
								 (g_groupBreak ? this_templateset->szTemplates[TMPL_GRPSTARTIN] : this_templateset->szTemplates[TMPL_GRPINNERIN]);
		else
			szTemplate = isSent ? this_templateset->szTemplates[TMPL_MSGOUT] : this_templateset->szTemplates[TMPL_MSGIN];
	}

	iTemplateLen = _tcslen(szTemplate);
	showTime = dwEffectiveFlags & MWF_LOG_SHOWTIME;
	showDate = dwEffectiveFlags & MWF_LOG_SHOWDATES;

	if (dat->hHistoryEvents) {
		if (dat->curHistory == dat->maxHistory) {
			MoveMemory(dat->hHistoryEvents, &dat->hHistoryEvents[1], sizeof(HANDLE) * (dat->maxHistory - 1));
			dat->curHistory--;
		}
		dat->hHistoryEvents[dat->curHistory++] = hDbEvent;
	}

	AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\ul0\\b0\\i0 ");

	while (i < iTemplateLen) {
		ci = szTemplate[i];
		if (ci == '%') {
			cc = szTemplate[i + 1];
			skipToNext = FALSE;
			skipFont = FALSE;
			/*
			 * handle modifiers
			 */
			while (cc == '#' || cc == '$' || cc == '&' || cc == '?' || cc == '\\') {
				switch (cc) {
					case '#':
						if (!dat->isHistory) {
							skipToNext = TRUE;
							goto skip;
						} else {
							i++;
							cc = szTemplate[i + 1];
							continue;
						}
					case '$':
						if (dat->isHistory) {
							skipToNext = TRUE;
							goto skip;
						} else {
							i++;
							cc = szTemplate[i + 1];
							continue;
						}
					case '&':
						i++;
						cc = szTemplate[i + 1];
						skipFont = TRUE;
						break;
					case '?':
						if (dwEffectiveFlags & MWF_LOG_NORMALTEMPLATES) {
							i++;
							cc = szTemplate[i + 1];
							continue;
						} else {
							i++;
							skipToNext = TRUE;
							goto skip;
						}
					case '\\':
						if (!(dwEffectiveFlags & MWF_LOG_NORMALTEMPLATES)) {
							i++;
							cc = szTemplate[i + 1];
							continue;
						} else {
							i++;
							skipToNext = TRUE;
							goto skip;
						}
				}
			}
			switch (cc) {
				case 'V':
					//AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\fs0\\\expnd-40 ~-%d-~", hDbEvent);
					break;
				case 'I': {
					if (dwEffectiveFlags & MWF_LOG_SHOWICONS) {
						int icon;
						if ((dwEffectiveFlags & MWF_LOG_INOUTICONS) && dbei.eventType == EVENTTYPE_MESSAGE)
							icon = isSent ? LOGICON_OUT : LOGICON_IN;
						else {
							switch (dbei.eventType) {
								case EVENTTYPE_URL:
									icon = LOGICON_URL;
									break;
								case EVENTTYPE_FILE:
									icon = LOGICON_FILE;
									break;
								case EVENTTYPE_ERRMSG:
									icon = LOGICON_ERROR;
									break;
								default:
									icon = LOGICON_MSG;
									break;
							}
							if (fIsStatusChangeEvent)
								icon = LOGICON_STATUS;
						}
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s\\fs1  #~#%01d%c%s ", GetRTFFont(MSGFONTID_SYMBOLS_IN), icon, isSent ? '>' : '<', GetRTFFont(isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset));
					} else
						skipToNext = TRUE;
					break;
				}
				case 'D':           // long date
					if (showTime && showDate) {
						szFinalTimestamp = Template_MakeRelativeDate(dat, final_time, g_groupBreak, (TCHAR)'D');
						AppendTimeStamp(szFinalTimestamp, isSent, &buffer, &bufferEnd, &bufferAlloced, skipFont, dat, iFontIDOffset);
					} else
						skipToNext = TRUE;
					break;
				case 'E':           // short date...
					if (showTime && showDate) {
						szFinalTimestamp = Template_MakeRelativeDate(dat, final_time, g_groupBreak, (TCHAR)'E');
						AppendTimeStamp(szFinalTimestamp, isSent, &buffer, &bufferEnd, &bufferAlloced, skipFont, dat, iFontIDOffset);
					} else
						skipToNext = TRUE;
					break;
				case 'a':           // 12 hour
				case 'h':           // 24 hour
					if (showTime) {
						if (skipFont)
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, cc == 'h' ? "%02d" : "%2d", cc == 'h' ? event_time.tm_hour : (event_time.tm_hour > 12 ? event_time.tm_hour - 12 : event_time.tm_hour));
						else
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, cc == 'h' ? "%s %02d" : "%s %2d", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset), cc == 'h' ? event_time.tm_hour : (event_time.tm_hour > 12 ? event_time.tm_hour - 12 : event_time.tm_hour));
					} else
						skipToNext = TRUE;
					break;
				case 'm':           // minute
					if (showTime) {
						if (skipFont)
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%02d", event_time.tm_min);
						else
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %02d", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset), event_time.tm_min);
					} else
						skipToNext = TRUE;
					break;
				case 's':           //second
					if (showTime && dwEffectiveFlags & MWF_LOG_SHOWSECONDS) {
						if (skipFont)
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%02d", event_time.tm_sec);
						else
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %02d", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset), event_time.tm_sec);
					} else
						skipToNext = TRUE;
					break;
				case 'p':            // am/pm symbol
					if (showTime) {
						if (skipFont)
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", event_time.tm_hour > 11 ? "PM" : "AM");
						else
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %s", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset), event_time.tm_hour > 11 ? "PM" : "AM");
					} else
						skipToNext = TRUE;
					break;
				case 'o':            // month
					if (showTime && showDate) {
						if (skipFont)
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%02d", event_time.tm_mon + 1);
						else
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %02d", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset), event_time.tm_mon + 1);
					} else
						skipToNext = TRUE;
					break;
				case'O':            // month (name)
					if (showTime && showDate) {
#ifdef _UNICODE
						if (skipFont)
							AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, months_translated[event_time.tm_mon], MAKELONG(isSent, dat->isHistory));
						else {
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset));
							AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, months_translated[event_time.tm_mon], MAKELONG(isSent, dat->isHistory));
						}
#else
						if (skipFont)
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", months_translated[event_time.tm_mon]);
						else
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %s", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset), months_translated[event_time.tm_mon]);
#endif
					} else
						skipToNext = TRUE;
					break;
				case 'd':           // day of month
					if (showTime && showDate) {
						if (skipFont)
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%02d", event_time.tm_mday);
						else
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %02d", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset), event_time.tm_mday);
					} else
						skipToNext = TRUE;
					break;
				case 'w':           // day of week
					if (showTime && showDate) {
#ifdef _UNICODE
						if (skipFont)
							AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, weekDays_translated[event_time.tm_wday], MAKELONG(isSent, dat->isHistory));
						else {
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset));
							AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, weekDays_translated[event_time.tm_wday], MAKELONG(isSent, dat->isHistory));
						}
#else
						if (skipFont)
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", weekDays_translated[event_time.tm_wday]);
						else
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %s", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset), weekDays_translated[event_time.tm_wday]);
#endif
					} else
						skipToNext = TRUE;
					break;
				case 'y':           // year
					if (showTime && showDate) {
						if (skipFont)
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%04d", event_time.tm_year + 1900);
						else
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %04d", GetRTFFont(isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset), event_time.tm_year + 1900);
					} else
						skipToNext = TRUE;
					break;
				case 'R':
				case 'r':           // long date
					if (showTime && showDate) {
						szFinalTimestamp = Template_MakeRelativeDate(dat, final_time, g_groupBreak, cc);
						AppendTimeStamp(szFinalTimestamp, isSent, &buffer, &bufferEnd, &bufferAlloced, skipFont, dat, iFontIDOffset);
					} else
						skipToNext = TRUE;
					break;
				case 't':
				case 'T':
					if (showTime) {
							szFinalTimestamp = Template_MakeRelativeDate(dat, final_time, g_groupBreak, (TCHAR)((dwEffectiveFlags & MWF_LOG_SHOWSECONDS) ? cc : (TCHAR)'t'));
							AppendTimeStamp(szFinalTimestamp, isSent, &buffer, &bufferEnd, &bufferAlloced, skipFont, dat, iFontIDOffset);
					} else
						skipToNext = TRUE;
					break;
				case 'S': {         // symbol
					if (dwEffectiveFlags & MWF_LOG_SYMBOLS) {
						if ((dwEffectiveFlags & MWF_LOG_INOUTICONS) && dbei.eventType == EVENTTYPE_MESSAGE)
							c = isSent ? 0x37 : 0x38;
						else {
							switch (dbei.eventType) {
								case EVENTTYPE_MESSAGE:
									c = (char)0xaa;
									break;
								case EVENTTYPE_FILE:
									c = (char)0xcd;
									break;
								case EVENTTYPE_URL:
									c = (char)0xfe;
									break;
								case EVENTTYPE_ERRMSG:
									c = (char)0x72;;
							}
							if (fIsStatusChangeEvent)
								c = 0x4e;
						}
						if (skipFont)
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%c%s ", c, GetRTFFont(isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset));
						else
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %c%s ", isSent ? GetRTFFont(MSGFONTID_SYMBOLS_OUT) : GetRTFFont(MSGFONTID_SYMBOLS_IN), c, GetRTFFont(isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset));
					} else
						skipToNext = TRUE;
					break;
				}
				case 'n':           // hard line break
					AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, dbei.flags & DBEF_RTL ? "\\rtlpar\\par\\rtlpar" : "\\par\\ltrpar");
					break;
				case 'l':           // soft line break
					AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\line");
					break;
				case 'N': {         // nickname
					if (heFlags != -1 && !(heFlags & HISTORYEVENTS_FLAG_EXPECT_CONTACT_NAME_BEFORE))
						break;

#if !defined(_UNICODE)
					szName = isSent ? szMyName : szYourName;
#endif
					if (!skipFont)
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", GetRTFFont(isSent ? MSGFONTID_MYNAME + iFontIDOffset : MSGFONTID_YOURNAME + iFontIDOffset));
#if defined(_UNICODE)
					if (isSent)
						AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, szMyName, MAKELONG(isSent, dat->isHistory));
					//AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, "%s", szMyName);
					else
						AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, szYourName, MAKELONG(isSent, dat->isHistory));
#else
					AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, "%s", szName);
#endif
					break;
				}
				case 'U':            // UIN
					if (!skipFont)
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", GetRTFFont(isSent ? MSGFONTID_MYNAME + iFontIDOffset : MSGFONTID_YOURNAME + iFontIDOffset));
					AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, "%s", isSent ? dat->myUin : dat->uin);
					break;
				case 'e':           // error message
#if defined(_UNICODE)
					AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", GetRTFFont(MSGFONTID_ERROR));
					AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, (wchar_t *)dbei.szModule, MAKELONG(isSent, dat->isHistory));
#else
					AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s %s", GetRTFFont(MSGFONTID_ERROR), dbei.szModule);
#endif
					break;
				case 'M': {         // message
					if (fIsStatusChangeEvent)
						dbei.eventType = EVENTTYPE_STATUSCHANGE;
					switch (dbei.eventType) {
						case EVENTTYPE_MESSAGE:
						case EVENTTYPE_ERRMSG:
						case EVENTTYPE_STATUSCHANGE: {
							if (fIsStatusChangeEvent || dbei.eventType == EVENTTYPE_ERRMSG) {
								if (dbei.eventType == EVENTTYPE_ERRMSG && dbei.cbBlob == 0)
									break;
								if (dbei.eventType == EVENTTYPE_ERRMSG) {
									if (!skipFont)
										AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\line%s ", GetRTFFont(fIsStatusChangeEvent ? H_MSGFONTID_STATUSCHANGES : MSGFONTID_MYMSG));
									else
										AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\line ");
								} else  {
									if (!skipFont)
										AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", GetRTFFont(fIsStatusChangeEvent ? H_MSGFONTID_STATUSCHANGES : MSGFONTID_MYMSG));
								}
							} else {
								if (!skipFont)
									AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", GetRTFFont(isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset));
							}

							if (rtfMessage != NULL) {
								AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", rtfMessage);
							} else {
#if defined( _UNICODE )
								AppendUnicodeToBuffer(&buffer, &bufferEnd, &bufferAlloced, formatted, MAKELONG(isSent, dat->isHistory));
#else   // unicode
								AppendToBufferWithRTF(MAKELONG(isSent, dat->isHistory), &buffer, &bufferEnd, &bufferAlloced, "%s", formatted);
#endif      // unicode
							}
							AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s", "\\b0\\ul0\\i0 ");
							break;
						}
						case EVENTTYPE_URL:
							if (!skipFont)
								AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", GetRTFFont(isSent ? MSGFONTID_MYMISC + iFontIDOffset : MSGFONTID_YOURMISC + iFontIDOffset));
							AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, "%s", dbei.pBlob);
							if ((dbei.pBlob + lstrlenA((char *)dbei.pBlob) + 1) != NULL && lstrlenA((char *)(dbei.pBlob + lstrlenA((char *)dbei.pBlob) + 1)))
								AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, " (%s)", dbei.pBlob + lstrlenA((char *)dbei.pBlob) + 1);
							break;
						case EVENTTYPE_FILE:
							if (!skipFont)
								AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", GetRTFFont(isSent ? MSGFONTID_MYMISC + iFontIDOffset : MSGFONTID_YOURMISC + iFontIDOffset));
							if ((dbei.pBlob + sizeof(DWORD) + lstrlenA((char *)(dbei.pBlob + sizeof(DWORD))) + 1) != NULL && lstrlenA((char *)(dbei.pBlob + sizeof(DWORD) + lstrlenA((char *)(dbei.pBlob + sizeof(DWORD))) + 1)))
								AppendToBufferWithRTF(0, &buffer, &bufferEnd, &bufferAlloced, "%s (%s)", dbei.pBlob + sizeof(DWORD), dbei.pBlob + sizeof(DWORD) + lstrlenA((char *)(dbei.pBlob + sizeof(DWORD))) + 1);
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
				case '-': {     // grid line
					TCHAR color = szTemplate[i + 2];
					if (color >= '0' && color <= '4') {
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\par\\sl-1\\slmult0\\highlight%d\\cf%d\\-\\par\\sl0", MSGDLGFONTCOUNT + 8 + (color - '0'), MSGDLGFONTCOUNT + 7 + (color - '0'));
						i++;
					} else {
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\par\\sl-1\\slmult0\\highlight%d\\cf%d\\-\\par\\sl0", MSGDLGFONTCOUNT + 4, MSGDLGFONTCOUNT + 4);
					}
					break;
				}
				case '~':       // font break (switch to default font...)
					AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, GetRTFFont(isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset));
					break;
				case 'H': {         // highlight
					TCHAR color = szTemplate[i + 2];

					if (color >= '0' && color <= '4') {
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight%d", MSGDLGFONTCOUNT + 8 + (color - '0'));
						i++;
					} else
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\highlight%d", (dwEffectiveFlags & MWF_LOG_INDIVIDUALBKG) ? (MSGDLGFONTCOUNT + (dat->isHistory?5:1) + ((isSent) ? 1 : 0)) : MSGDLGFONTCOUNT + 3);
					break;
				}
				case '|':       // tab
					if (dwEffectiveFlags & MWF_LOG_INDENT)
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\tab");
					else
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, " ");
					break;
				case 'f': {     // font tag...
					TCHAR code = szTemplate[i + 2];
					int fontindex = -1;
					switch (code) {
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
					if (fontindex != -1) {
						i++;
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "%s ", GetRTFFont(fontindex));
					} else
						skipToNext = TRUE;
					break;
				}
				case 'c': {     // font color (using one of the predefined 5 colors) or one of the standard font colors (m = message, d = date/time, n = nick)
					TCHAR color = szTemplate[i + 2];
					if (color >= '0' && color <= '4') {
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\cf%d ", MSGDLGFONTCOUNT + 8 + (color - '0'));
						i++;
					} else if (color == (TCHAR)'d') {
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\cf%d ", isSent ? MSGFONTID_MYTIME + iFontIDOffset : MSGFONTID_YOURTIME + iFontIDOffset);
						i++;
					} else if (color == (TCHAR)'m') {
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\cf%d ", isSent ? MSGFONTID_MYMSG + iFontIDOffset : MSGFONTID_YOURMSG + iFontIDOffset);
						i++;
					} else if (color == (TCHAR)'n') {
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\cf%d ", isSent ? MSGFONTID_MYNAME + iFontIDOffset : MSGFONTID_YOURNAME + iFontIDOffset);
						i++;
					} else if (color == (TCHAR)'s') {
						AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\cf%d ", isSent ? MSGFONTID_SYMBOLS_OUT : MSGFONTID_SYMBOLS_IN);
						i++;
					} else
						skipToNext = TRUE;
					break;
				}
				case '<':		// bidi tag
					AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\rtlmark\\rtlch ");
					break;
				case '>':		// bidi tag
					AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\ltrmark\\ltrch ");
					break;
			}
	skip:
			if (skipToNext) {
				i++;
				while (szTemplate[i] != '%' && i < iTemplateLen) i++;
			} else
				i += 2;
		} else {
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

	if (dat->hHistoryEvents)
		AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, dat->szMicroLf, MSGDLGFONTCOUNT + 1 + ((isSent) ? 1 : 0), hDbEvent);

	AppendToBuffer(&buffer, &bufferEnd, &bufferAlloced, "\\par");

	if (streamData->dbei == 0)
		free(dbei.pBlob);
	HistoryEvents_ReleaseText(rtfMessage);

	dat->iLastEventType = MAKELONG((dbei.flags & (DBEF_SENT | DBEF_READ | DBEF_RTL)), dbei.eventType);
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
				if (dat->buffer) free(dat->buffer);
				dat->buffer = CreateRTFHeader(dat->dlgDat);
				dat->stage = STREAMSTAGE_EVENTS;
				break;
			case STREAMSTAGE_EVENTS:
				if (dat->eventsToInsert) {
					do {
						if (dat->buffer) free(dat->buffer);
						dat->buffer = Template_CreateRTFFromDbEvent(dat->dlgDat, dat->hContact, dat->hDbEvent, !dat->isEmpty, dat);
						if (dat->buffer)
							dat->hDbEventLast = dat->hDbEvent;
						dat->hDbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, (WPARAM) dat->hDbEvent, 0);
						if (--dat->eventsToInsert == 0)
							break;
					} while (dat->buffer == NULL && dat->hDbEvent);
					if (dat->buffer) {
						//dat->isEmpty = 0;
						break;
					}
				}
				dat->stage = STREAMSTAGE_TAIL;
				//fall through
			case STREAMSTAGE_TAIL: {
				if (dat->buffer) free(dat->buffer);
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

static void SetupLogFormatting(struct MessageWindowData *dat)
{
	if (dat->hHistoryEvents) {
		mir_snprintf(dat->szMicroLf, sizeof(dat->szMicroLf), "%s", "\\v\\cf%d \\ ~-+%d+-~\\v0 ");
	} else {
		mir_snprintf(dat->szMicroLf, sizeof(dat->szMicroLf), "%s\\par\\ltrpar\\sl-1%s ", GetRTFFont(MSGDLGFONTCOUNT), GetRTFFont(MSGDLGFONTCOUNT));
	}
}

void StreamInEvents(HWND hwndDlg, HANDLE hDbEventFirst, int count, int fAppend, DBEVENTINFO *dbei_s)
{
	EDITSTREAM stream = { 0 };
	struct LogStreamData streamData = {	0 };
	struct MessageWindowData *dat = (struct MessageWindowData *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	CHARRANGE oldSel, sel;
	HWND hwndrtf;
	LONG startAt = 0;
	FINDTEXTEXA fi;
	struct tm tm_now, tm_today;
	time_t now;
	SCROLLINFO si = {0}, *psi = &si;
	POINT pt = {0};
	BOOL  wasFirstAppend = (dat->isAutoRTL & 2) ? TRUE : FALSE;
	BOOL  isSent;


	/*
	 * calc time limit for grouping
	 */

	hwndrtf = dat->hwndIEView ? dat->hwndIWebBrowserControl : GetDlgItem(hwndDlg, IDC_LOG);

	si.cbSize = sizeof(si);
	if (IsWindow(hwndrtf)) {
		si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;;
		GetScrollInfo(hwndrtf, SB_VERT, &si);
		SendMessage(hwndrtf, EM_GETSCROLLPOS, 0, (LPARAM) &pt);

		if (GetWindowLongPtr(hwndrtf, GWL_STYLE) & WS_VSCROLL)
			psi = &si;
		else
			psi = NULL;
	}

	rtfFonts = dat->theme.rtfFonts ? dat->theme.rtfFonts : &(rtfFontsGlobal[0][0]);
	now = time(NULL);

	tm_now = *localtime(&now);
	tm_today = tm_now;
	tm_today.tm_hour = tm_today.tm_min = tm_today.tm_sec = 0;
	today = mktime(&tm_today);

	if (dat->hwndIEView != 0) {
		IEVIEWEVENT event;

		event.cbSize = sizeof(IEVIEWEVENT);
		event.hwnd = dat->hwndIEView;
		event.hContact = dat->hContact;
#if defined(_UNICODE)
		event.dwFlags = (dat->dwFlags & MWF_LOG_RTL) ? IEEF_RTL : 0;
		if (dat->sendMode & SMODE_FORCEANSI) {
			event.dwFlags |= IEEF_NO_UNICODE;
			event.codepage = dat->codePage;
		} else
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
		DM_ScrollToBottom(hwndDlg, dat, 0, 0);
		if (fAppend)
			dat->hDbEventLast = hDbEventFirst;
		else
			dat->hDbEventLast = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)dat->hContact, 0);
		return;
	}
	if (dat->hwndHPP != 0) {
		IEVIEWEVENT event;

		event.cbSize = sizeof(IEVIEWEVENT);
		event.hwnd = dat->hwndHPP;
		event.hContact = dat->hContact;
#if defined(_UNICODE)
		event.dwFlags = (dat->dwFlags & MWF_LOG_RTL) ? IEEF_RTL : 0;
		if (dat->sendMode & SMODE_FORCEANSI) {
			event.dwFlags |= IEEF_NO_UNICODE;
			event.codepage = dat->codePage;
		} else
			event.codepage = 0;
#else
		event.dwFlags = ((dat->dwFlags & MWF_LOG_RTL) ? IEEF_RTL : 0) | IEEF_NO_UNICODE;
		event.codepage = 0;
#endif
		if (!fAppend) {
			event.iType = IEE_CLEAR_LOG;
			CallService(MS_HPP_EG_EVENT, 0, (LPARAM)&event);
		}
		event.iType = IEE_LOG_EVENTS;
		event.hDbEventFirst = hDbEventFirst;
		event.count = count;
		CallService(MS_HPP_EG_EVENT, 0, (LPARAM)&event);
		//SendMessage(hwndDlg, DM_FORCESCROLL, (WPARAM)&pt, (LPARAM)&si);
		DM_ScrollToBottom(hwndDlg, dat, 0, 0);
		if (fAppend)
			dat->hDbEventLast = hDbEventFirst;
		else
			dat->hDbEventLast = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)dat->hContact, 0);
		return;
	}

	// separator strings used for grid lines, message separation and so on...

	dat->clr_added = FALSE;

	if (dat->szMicroLf[0] == 0)
		SetupLogFormatting(dat);

	szYourName = dat->szNickname;
	szMyName = dat->szMyNickname;

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
	streamData.isAppend = fAppend;

	if (fAppend) {
		GETTEXTLENGTHEX gtxl = {0};
#if defined(_UNICODE)
		gtxl.codepage = 1200;
		gtxl.flags = GTL_DEFAULT | GTL_PRECISE | GTL_NUMCHARS;
#else
		gtxl.codepage = CP_ACP;
		gtxl.flags = GTL_DEFAULT | GTL_PRECISE;
#endif
		fi.chrg.cpMin = SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETTEXTLENGTHEX, (WPARAM) & gtxl, 0);
		sel.cpMin = sel.cpMax = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LOG));
		SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXSETSEL, 0, (LPARAM) & sel);
	} else {
		SetDlgItemText(hwndDlg, IDC_LOG, _T(""));
		sel.cpMin = 0;
		sel.cpMax = GetWindowTextLength(hwndrtf);
		SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXSETSEL, 0, (LPARAM) &sel);
		fi.chrg.cpMin = 0;
		dat->isAutoRTL = 0;
	}
	startAt = fi.chrg.cpMin;

	SendMessage(hwndrtf, WM_SETREDRAW, FALSE, 0);

	SendDlgItemMessage(hwndDlg, IDC_LOG, EM_STREAMIN, fAppend ? SFF_SELECTION | SF_RTF : SFF_SELECTION |  SF_RTF, (LPARAM) & stream);
	//SendDlgItemMessage(hwndDlg, IDC_LOG, EM_STREAMIN, fAppend ? SFF_SELECTION | SF_RTF : SF_RTF, (LPARAM) & stream);
	SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXSETSEL, 0, (LPARAM) & oldSel);
	SendDlgItemMessage(hwndDlg, IDC_LOG, EM_HIDESELECTION, FALSE, 0);
	dat->hDbEventLast = streamData.hDbEventLast;

	if (dat->isAutoRTL & 1) {
		if (dat->dwFlags & MWF_LOG_INDIVIDUALBKG)
 			SendMessage(hwndrtf, EM_SETBKGNDCOLOR, 0, LOWORD(dat->iLastEventType) & DBEF_SENT ? (fAppend?dat->theme.outbg:dat->theme.oldoutbg) : (fAppend?dat->theme.inbg:dat->theme.oldinbg));
		else
			SendMessage(hwndrtf, EM_SETBKGNDCOLOR, 0, dat->theme.bg);
	}

	if (!(dat->isAutoRTL & 1)) {
		GETTEXTLENGTHEX gtxl = {0};
		PARAFORMAT2 pf2 = {0};

#if defined(_UNICODE)
		gtxl.codepage = 1200;
		gtxl.flags = GTL_DEFAULT | GTL_PRECISE | GTL_NUMCHARS;
#else
		gtxl.codepage = CP_ACP;
		gtxl.flags = GTL_DEFAULT | GTL_PRECISE;
#endif
		sel.cpMax = SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETTEXTLENGTHEX, (WPARAM) & gtxl, 0);
		sel.cpMin = sel.cpMax - 1;
		SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXSETSEL, 0, (LPARAM) & sel);
		SendDlgItemMessage(hwndDlg, IDC_LOG, EM_REPLACESEL, FALSE, (LPARAM)_T(""));
		dat->isAutoRTL |= 2;
	}

	if (streamData.dbei != 0)
		isSent = (streamData.dbei->flags & DBEF_SENT) != 0;
	else {
		DBEVENTINFO dbei = {0};
		dbei.cbSize = sizeof(dbei);
		CallService(MS_DB_EVENT_GET, (WPARAM) hDbEventFirst, (LPARAM)&dbei);
		isSent = (dbei.flags & DBEF_SENT) != 0;
	}

	ReplaceIcons(hwndDlg, dat, startAt, fAppend, isSent);
	dat->clr_added = FALSE;

	SendMessage(hwndDlg, DM_FORCESCROLL, (WPARAM)&pt, (LPARAM)psi);
	SendDlgItemMessage(hwndDlg, IDC_LOG, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(GetDlgItem(hwndDlg, IDC_LOG), NULL, FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_QUOTE), dat->hDbEventLast != NULL);
	if (streamData.buffer) free(streamData.buffer);
}

static void ReplaceIcons(HWND hwndDlg, struct MessageWindowData *dat, LONG startAt, int fAppend, BOOL isSent)
{
	FINDTEXTEXA fi;
	CHARFORMAT2 cf2;
	HWND hwndrtf;
	IRichEditOle *ole;
	TEXTRANGEA tr;
	COLORREF crDefault;
	struct MsgLogIcon theIcon;
	char trbuffer[40];
	DWORD dwScale = DBGetContactSettingDword(NULL, SRMSGMOD_T, "iconscale", 0);
	tr.lpstrText = trbuffer;

	hwndrtf = GetDlgItem(hwndDlg, IDC_LOG);
	fi.chrg.cpMin = startAt;
	if (dat->clr_added) {
		unsigned int length;
		int index;
		CHARRANGE cr;
		fi.lpstrText = "##col##";
		fi.chrg.cpMax = -1;
		ZeroMemory((void *)&cf2, sizeof(cf2));
		cf2.cbSize = sizeof(cf2);
		cf2.dwMask = CFM_COLOR;
		while (SendMessageA(hwndrtf, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&fi) > -1) {
			tr.chrg.cpMin = fi.chrgText.cpMin;
			tr.chrg.cpMax = tr.chrg.cpMin + 18;
			trbuffer[0] = 0;
			SendMessageA(hwndrtf, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
			trbuffer[18] = 0;
			cr.cpMin = fi.chrgText.cpMin;
			cr.cpMax = cr.cpMin + 18;
			SendMessage(hwndrtf, EM_EXSETSEL, 0, (LPARAM)&cr);
			SendMessageA(hwndrtf, EM_REPLACESEL, FALSE, (LPARAM)"");
			length = (unsigned int)atol(&trbuffer[7]);
			index = atol(&trbuffer[14]);
			if (length > 0 && length < 20000 && index >= RTF_CTABLE_DEFSIZE && index < myGlobals.rtf_ctablesize) {
				cf2.crTextColor = rtf_ctable[index].clr;
				cr.cpMin = fi.chrgText.cpMin;
				cr.cpMax = cr.cpMin + length;
				SendMessage(hwndrtf, EM_EXSETSEL, 0, (LPARAM)&cr);
				SendMessage(hwndrtf, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
			}
		}
	}
	fi.chrg.cpMin = startAt;
	if (dat->dwFlags & MWF_LOG_SHOWICONS) {
		BYTE bIconIndex = 0;
		char bDirection = 0;
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
			bIconIndex = ((BYTE)trbuffer[0] - (BYTE)'0');
			if (bIconIndex >= NR_LOGICONS) {
				fi.chrg.cpMin = fi.chrgText.cpMax + 6;
				continue;
			}
			bDirection = trbuffer[1];
			SendMessage(hwndrtf, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
			crDefault = cf2.crBackColor == 0 ? (dat->dwFlags & MWF_LOG_INDIVIDUALBKG ? (bDirection == '>' ? (fAppend?dat->theme.outbg:dat->theme.oldoutbg) : (fAppend?dat->theme.inbg:dat->theme.oldinbg)) : dat->theme.bg) : cf2.crBackColor;
			CacheIconToBMP(&theIcon, Logicons[bIconIndex], crDefault, dwScale, dwScale);
			ImageDataInsertBitmap(ole, theIcon.hBmp);
			DeleteCachedIcon(&theIcon);
			fi.chrg.cpMin = cr.cpMax + 6;
		}
		ReleaseRichEditOle(ole);
	}
	/*
	 * do smiley replacing, using the service
	 */

	if (myGlobals.g_SmileyAddAvail) {
		CHARRANGE sel;
		SMADD_RICHEDIT3 smadd;

		sel.cpMin = startAt;
		sel.cpMax = -1;

		ZeroMemory(&smadd, sizeof(smadd));

		smadd.cbSize = sizeof(smadd);
		smadd.hwndRichEditControl = GetDlgItem(hwndDlg, IDC_LOG);
		smadd.Protocolname = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
		smadd.hContact = dat->bIsMeta ? dat->hSubContact : dat->hContact;
		smadd.flags = isSent ? SAFLRE_OUTGOING : 0;

		if (startAt > 0)
			smadd.rangeToReplace = &sel;
		else
			smadd.rangeToReplace = NULL;
		smadd.disableRedraw = TRUE;
		if (dat->doSmileys)
			CallService(MS_SMILEYADD_REPLACESMILEYS, TABSRMM_SMILEYADD_BKGCOLORMODE, (LPARAM)&smadd);
	}

	if (myGlobals.m_MathModAvail) {
		TMathRicheditInfo mathReplaceInfo;
		CHARRANGE mathNewSel;
		mathNewSel.cpMin = startAt;
		mathNewSel.cpMax = -1;
		mathReplaceInfo.hwndRichEditControl = GetDlgItem(hwndDlg, IDC_LOG);
		if (startAt > 0) mathReplaceInfo.sel = & mathNewSel;
		else mathReplaceInfo.sel = 0;
		mathReplaceInfo.disableredraw = TRUE;
		CallService(MATH_RTF_REPLACE_FORMULAE, 0, (LPARAM)&mathReplaceInfo);
	}

	if (dat->hHistoryEvents && dat->curHistory == dat->maxHistory) {
		char szPattern[50];
		FINDTEXTEXA fi;

		_snprintf(szPattern, 40, "~-+%d+-~", dat->hHistoryEvents[0]);
		fi.lpstrText = szPattern;
		fi.chrg.cpMin = 0;
		fi.chrg.cpMax = -1;
		if (SendMessageA(hwndrtf, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&fi) != 0) {
			CHARRANGE sel;
			sel.cpMin = 0;
			sel.cpMax = 20;
			SendMessage(hwndrtf, EM_SETSEL, 0, fi.chrgText.cpMax + 1);
			SendMessageA(hwndrtf, EM_REPLACESEL, TRUE, (LPARAM)"");
		}
	}
}

/*
 * NLS functions (for unicode version only) encoding stuff..
 */

static BOOL CALLBACK LangAddCallback(LPTSTR str)
{
	int i, count;
	UINT cp;

	cp = _ttoi(str);
	count = sizeof(cpTable) / sizeof(cpTable[0]);
	for (i = 0; i < count && cpTable[i].cpId != cp; i++);
	if (i < count) {
		AppendMenu(myGlobals.g_hMenuEncoding, MF_STRING, cp, TranslateTS(cpTable[i].cpName));
	}
	return TRUE;
}

void BuildCodePageList()
{
	myGlobals.g_hMenuEncoding = CreateMenu();
	AppendMenu(myGlobals.g_hMenuEncoding, MF_STRING, 500, TranslateT("Use default codepage"));
	AppendMenuA(myGlobals.g_hMenuEncoding, MF_SEPARATOR, 0, 0);
	EnumSystemCodePages(LangAddCallback, CP_INSTALLED);
}

static TCHAR *Template_MakeRelativeDate(struct MessageWindowData *dat, time_t check, int groupBreak, TCHAR code)
{
	static TCHAR szResult[100];
	DBTIMETOSTRINGT dbtts;
	szResult[0] = 0;
	dbtts.cbDest = 70;;
	dbtts.szDest = szResult;

	if ((code == (TCHAR)'R' || code == (TCHAR)'r') && check >= today) {
		lstrcpy(szResult, szToday);
	} else if ((code == (TCHAR)'R' || code == (TCHAR)'r') && check > (today - 86400)) {
		lstrcpy(szResult, szYesterday);
	} else {
		if (code == (TCHAR)'D' || code == (TCHAR)'R')
			dbtts.szFormat = _T("D");
		else if (code == (TCHAR)'T')
			dbtts.szFormat = _T("s");
		else if (code == (TCHAR)'t')
			dbtts.szFormat = _T("t");
		else
			dbtts.szFormat = _T("d");
		CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, check, (LPARAM)&dbtts);
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
	size_t i, len;
	char *p;
	WCHAR *wszTemp = NULL;

	if (str == NULL)
		return NULL;

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
		} else {
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
	size_t len, i;
	const WCHAR *wszTemp, *w;

	if (str == NULL)
		return NULL;

	wszTemp = str;

	// Convert unicode to utf8
	len = 0;
	for (w = wszTemp; *w; w++) {
		if (*w < 0x0080) len++;
		else if (*w < 0x0800) len += 2;
		else len += 3;
	}

	if ((szOut = (unsigned char *) malloc(len + 2)) == NULL)
		return NULL;

	i = 0;
	for (w = wszTemp; *w; w++) {
		if (*w < 0x0080)
			szOut[i++] = (unsigned char) * w;
		else if (*w < 0x0800) {
			szOut[i++] = 0xc0 | ((*w) >> 6);
			szOut[i++] = 0x80 | ((*w) & 0x3f);
		} else {
			szOut[i++] = 0xe0 | ((*w) >> 12);
			szOut[i++] = 0x80 | (((*w) >> 6) & 0x3f);
			szOut[i++] = 0x80 | ((*w) & 0x3f);
		}
	}
	szOut[i] = '\0';
	return (char *) szOut;
}
#endif
