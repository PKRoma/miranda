/*
Chat module plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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

#include "chat.h"
#include <math.h>
#include <mbstring.h>
#include <shlwapi.h>


// The code for streaming the text is to a large extent copied from
// the srmm module and then modified to fit the chat module.

extern FONTINFO aFonts[OPTIONS_FONTCOUNT];
extern HICON	hIcons[OPTIONS_ICONCOUNT];
extern BOOL		SmileyAddInstalled;

static PBYTE pLogIconBmpBits[OPTIONS_ICONCOUNT-11];
static int logIconBmpSize[sizeof(pLogIconBmpBits) / sizeof(pLogIconBmpBits[0])];

typedef struct {
    char *	buffer;
    int		bufferOffset, bufferLen;
	HWND	hwnd;
	LOGINFO* lin;
	int		iColCount;
	BOOL	bStripFormat;
	CHATWINDOWDATA *data;
} LOGSTREAMDATA;

static int logPixelSY;
static int logPixelSX;

static int EventToIndex(LOGINFO * lin)
{
	switch(lin->iType)
	{
	case GC_EVENT_MESSAGE: 
		{
		if(lin->bIsMe) 
			return 10;
		else 
			return 9;
		}
	case GC_EVENT_JOIN: return 3;
	case GC_EVENT_PART: return 4;
	case GC_EVENT_QUIT: return 5;
	case GC_EVENT_NICK: return 7;
	case GC_EVENT_KICK: return 6;
	case GC_EVENT_NOTICE: return 8;
	case GC_EVENT_TOPIC: return 11;
	case GC_EVENT_INFORMATION:return 12;
	case GC_EVENT_ADDSTATUS: return 13;
	case GC_EVENT_REMOVESTATUS: return 14;
	case GC_EVENT_ACTION: return 15;
	default:break;
	}
	return 0;
}

void ValidateFilename (char * filename)
{
	char *p1 = filename;
	char szForbidden[] = "\\/:*?\"<>|";
	while(*p1 != '\0')
	{
		if(strchr(szForbidden, *p1))
			*p1 = '_';
		p1 +=1;
	}
}

static char *Log_SetStyle(int style, int fontindex)
{
    static char szStyle[128];
    _snprintf(szStyle, sizeof(szStyle), "\\f%u\\cf%u\\ul0\\highlight0\\b%d\\i%d\\fs%u", style, style+1, aFonts[fontindex].lf.lfWeight >= FW_BOLD ? 1 : 0, aFonts[fontindex].lf.lfItalic, 2 * abs(aFonts[fontindex].lf.lfHeight) * 74 / logPixelSY);
   return szStyle;
}

static void Log_Append(char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
{
	va_list va;
	int charsDone = 0;

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

static int Log_AppendRTF(LOGSTREAMDATA* streamData,char **buffer, int *cbBufferEnd, int *cbBufferAlloced, const char *fmt, ...)
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
            MoveMemory(*buffer + i + 6, *buffer + i + 2, *cbBufferEnd - i - 1);
            CopyMemory(*buffer + i, "\\line ", 6);
            *cbBufferEnd += 4;
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

		else if ( (*buffer)[i]=='%') 
		{
			char szTemp[200];
			int iLen = 0;
			int iOldCount = 0;

			szTemp[0] = '\0';		

			switch ((*buffer)[i + 1])
			{
			case '%':
				_snprintf(szTemp, sizeof(szTemp), "%s", "%");
				iOldCount = 2;
				break;
			case 'c':
			case 'f':
				if(g_LogOptions.StripFormat || streamData->bStripFormat)
					szTemp[0] = '\0';
				else
				{
					_snprintf(szTemp, sizeof(szTemp), ((*buffer)[i + 1]=='c')?"\\cf%u ":"\\highlight%u ", streamData->iColCount);
					streamData->iColCount++;
				}
				iOldCount = 11;
				break;
			case 'C':
			case 'F':
				if(g_LogOptions.StripFormat || streamData->bStripFormat)
					szTemp[0] = '\0';
				else
				{
					_snprintf(szTemp, sizeof(szTemp), ((*buffer)[i + 1]=='C')?"\\cf2 ":"\\highlight0 ");
				}
				iOldCount = 2;
				break;
			case 'b':
			case 'u':
			case 'i':
				if(streamData->bStripFormat)
					szTemp[0] = '\0';
				else
				{
					_snprintf(szTemp, sizeof(szTemp), (*buffer)[i + 1] == 'u'?"\\%cl ":"\\%c ", (*buffer)[i + 1]);
				}
				iOldCount = 2;
				break;
			case 'B':
			case 'U':
			case 'I':
				if(streamData->bStripFormat)
					szTemp[0] = '\0';
				else
				{
					_snprintf(szTemp, sizeof(szTemp), (*buffer)[i + 1] == 'U'?"\\%cl0 ":"\\%c0 ", (char)CharLower((LPTSTR)(*buffer)[i + 1]));
				}
				iOldCount = 2;
				break;
			case 'r':
				if(streamData->bStripFormat)
					szTemp[0] = '\0';
				else
				{
					_snprintf(szTemp, sizeof(szTemp), "%s ", Log_SetStyle(2, EventToIndex(streamData->lin)));
				}
				iOldCount = 2;
				break;
			default:break;
			}
			if(iOldCount)
			{
				iLen = lstrlen(szTemp);
				if (*cbBufferEnd + (iLen-iOldCount)+1 > *cbBufferAlloced) {
					*cbBufferAlloced += 1024;
					*buffer = (char *) realloc(*buffer, *cbBufferAlloced);
				}
				MoveMemory(*buffer + i + iLen, *buffer + i + iOldCount, *cbBufferEnd - i - iOldCount+1);
				if(iLen > 0)
				{
					CopyMemory(*buffer + i, szTemp, iLen);
					*cbBufferEnd += iLen - iOldCount;
					i += iLen;
					i -=1;
				}
				else
				{
					*cbBufferEnd -= iOldCount;
					i -= 1;
				}
			}

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
static void AddEventToBuffer(char **buffer, int *bufferEnd, int *bufferAlloced, LOGSTREAMDATA *streamData)
{
	char szTemp[512];
	char * pszNick = NULL;
	if(streamData->lin->pszNick )
	{
		if(streamData->lin->pszUserInfo)
			_snprintf(szTemp, sizeof(szTemp), "%s (%s)", streamData->lin->pszNick, streamData->lin->pszUserInfo);
		else
			_snprintf(szTemp, sizeof(szTemp), "%s", streamData->lin->pszNick);
		pszNick = szTemp;
	}

	if(streamData && streamData->lin)
	{
		switch(streamData->lin->iType)
		{
		case GC_EVENT_MESSAGE:
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, "%s", streamData->lin->pszText);break;
		case GC_EVENT_ACTION:
			if(streamData->lin->pszNick && streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("*%s %s*"), streamData->lin->pszNick, streamData->lin->pszText);break;
		case GC_EVENT_JOIN:
			if(pszNick)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s has joined"), pszNick);break;
		case GC_EVENT_PART:
			if(pszNick)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s has left"), pszNick);
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, ": %s", streamData->lin->pszText);
				break;
		case GC_EVENT_QUIT:
			if(pszNick)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s has disconnected"), pszNick);
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, ": %s", streamData->lin->pszText);
				break;
		case GC_EVENT_NICK:
			if(pszNick && streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s is now known as %s"), pszNick, streamData->lin->pszText);break;
		case GC_EVENT_KICK:
			if(streamData->lin->pszNick && streamData->lin->pszStatus)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s kicked %s"), streamData->lin->pszStatus, streamData->lin->pszNick);
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, ": %s", streamData->lin->pszText);
				break;
		case GC_EVENT_NOTICE:
			if(streamData->lin->pszNick && streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("Notice from %s: %s"), streamData->lin->pszNick, streamData->lin->pszText);break;
		case GC_EVENT_TOPIC:
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("The topic is \'%s%s\'"), streamData->lin->pszText, "%r");
			if(streamData->lin->pszNick)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate(" (set by %s)"), streamData->lin->pszNick);
			break;
		case GC_EVENT_INFORMATION:
			if(streamData->lin->pszText)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, streamData->lin->bIsMe?"--> %s":"%s", streamData->lin->pszText);break;
		case GC_EVENT_ADDSTATUS:
			if(streamData->lin->pszNick && streamData->lin->pszText && streamData->lin->pszStatus)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s enables \'%s\' status for %s"), streamData->lin->pszText, streamData->lin->pszStatus, streamData->lin->pszNick);break;
		case GC_EVENT_REMOVESTATUS:
			if(streamData->lin->pszNick && streamData->lin->pszText && streamData->lin->pszStatus)
				Log_AppendRTF(streamData, buffer, bufferEnd, bufferAlloced, Translate("%s disables \'%s\' status for %s"), streamData->lin->pszText , streamData->lin->pszStatus, streamData->lin->pszNick);break;
		default:break;
		}
	}

}

char *MakeTimeStamp(char * pszStamp, time_t time)
{

	static char szTime[30];
	strftime(szTime, 29, pszStamp, localtime(&time));
	return szTime;
}
static void FindColorsInText(char **buffer, int *bufferEnd, int *bufferAlloced, char *pszText)
{
	char * p1;
	if (pszText)
	{
		p1 = strchr(pszText, '%');
		while(p1)
		{
			p1++;
			switch(*p1)
			{
			case 'c':
			case 'f':
				{
				char szRed[4];
				char szGreen[4];
				char szBlue[4];

				p1++;
				lstrcpyn(szRed, p1, 4);
				lstrcpyn(szGreen, p1+3, 4);
				lstrcpyn(szBlue, p1+6, 4);
				Log_Append(buffer, bufferEnd, bufferAlloced, "\\red%s\\green%s\\blue%s;", szRed, szGreen, szBlue);
				}break;
			default:break;
			}
			p1++;
			p1 = strchr(p1, '%');
		}
	}
	return ;

}
static char* Log_CreateBody(LOGSTREAMDATA *streamData)
{
    HDC hdc;
	char *buffer;
    int bufferAlloced, bufferEnd, i, me = 0;

    bufferEnd = 0;
    bufferAlloced = 1024;
    buffer = (char *) malloc(bufferAlloced);
    buffer[0] = '\0';
	
	//get the number of pixels per logical inch
	hdc = GetDC(NULL);
	logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
	logPixelSX = GetDeviceCaps(hdc, LOGPIXELSX);
	ReleaseDC(NULL, hdc);

	//RTF HEADER
	i = streamData->lin->bIsHighlighted?16:EventToIndex(streamData->lin);
    Log_Append(&buffer, &bufferEnd, &bufferAlloced, "{\\rtf1\\ansi\\deff0{\\fonttbl");
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "{\\f0\\fnil\\fcharset%u %s;}", aFonts[0].lf.lfCharSet, aFonts[0].lf.lfFaceName);
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "{\\f1\\fnil\\fcharset%u %s;}", aFonts[streamData->lin->bIsMe?2:1].lf.lfCharSet, aFonts[streamData->lin->bIsMe?2:1].lf.lfFaceName);
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "{\\f2\\fnil\\fcharset%u %s;}", aFonts[i].lf.lfCharSet, aFonts[i].lf.lfFaceName);
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "}{\\colortbl ;");
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(aFonts[0].color), GetGValue(aFonts[0].color), GetBValue(aFonts[0].color));
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(aFonts[streamData->lin->bIsMe?2:1].color), GetGValue(aFonts[streamData->lin->bIsMe?2:1].color), GetBValue(aFonts[streamData->lin->bIsMe?2:1].color));
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\red%u\\green%u\\blue%u;", GetRValue(aFonts[i].color), GetGValue(aFonts[i].color), GetBValue(aFonts[i].color));
	FindColorsInText(&buffer, &bufferEnd, &bufferAlloced, streamData->lin->pszText);
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "}\\pard");
	{ //indent the text 
		int iIndent = 0;

		if(g_LogOptions.dwIconFlags)
		{
			iIndent += (14*1440)/logPixelSX;
			Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tx%u", iIndent);
		}
		if(g_LogOptions.ShowTime)
		{
			int iSize = (g_LogOptions.LogTextIndent*1440)/logPixelSX;
 			Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tx%u", iIndent + iSize );
			if(g_LogOptions.LogIndentEnabled)
				iIndent += iSize;
		}
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\fi-%u\\li%u", iIndent, iIndent);
	}

	//RTF BODY
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\par");
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", Log_SetStyle(0, 0 ));

	// Insert icon
	if (streamData->lin->iType&g_LogOptions.dwIconFlags || streamData->lin->bIsHighlighted&&g_LogOptions.dwIconFlags&GC_EVENT_HIGHLIGHT)
	{
		int iIndex = EventToIndex(streamData->lin);
		if (streamData->lin->bIsHighlighted&&g_LogOptions.dwIconFlags&GC_EVENT_HIGHLIGHT)
			iIndex = 16;
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\f0\\fs14");
		while (bufferAlloced - bufferEnd < logIconBmpSize[0])
			bufferAlloced += 1024;
		buffer = (char *) realloc(buffer, bufferAlloced);
		CopyMemory(buffer + bufferEnd, pLogIconBmpBits[iIndex-3], logIconBmpSize[iIndex-3]);
		bufferEnd += logIconBmpSize[iIndex-3];
	}

	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", Log_SetStyle(0, 0 ));

	if (g_LogOptions.dwIconFlags)
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tab ");

	//insert timestamp
	if(g_LogOptions.ShowTime)
	{
		char szTimeStamp[30];
		char szOldTimeStamp[30];
		
		lstrcpyn(szTimeStamp, MakeTimeStamp(g_LogOptions.pszTimeStamp, streamData->lin->time), 30);
		lstrcpyn(szOldTimeStamp, MakeTimeStamp(g_LogOptions.pszTimeStamp, streamData->data->LastTime), 30);
		if(!g_LogOptions.ShowTimeIfChanged || streamData->data->LastTime == 0 || lstrcmp(szTimeStamp, szOldTimeStamp))
		{
			streamData->data->LastTime = streamData->lin->time;
			Log_Append(&buffer, &bufferEnd, &bufferAlloced, szTimeStamp);
		}
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\\tab ");
	}

	// Insert the nick
	if(streamData->lin->pszNick && streamData->lin->iType == GC_EVENT_MESSAGE)
	{
		char pszTemp[30];
		char * p1;

		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", Log_SetStyle(1, streamData->lin->bIsMe?2:1));
		if(streamData->lin->bIsMe)
			lstrcpyn(pszTemp, g_LogOptions.pszOutgoingNick, 29);
		else 
			lstrcpyn(pszTemp, g_LogOptions.pszIncomingNick, 29);
		p1 = strchr(pszTemp, '%');
		if(p1 && p1[1] == 'n')
			p1[1] = 's';
		
		Log_AppendRTF(streamData, &buffer, &bufferEnd, &bufferAlloced, pszTemp, streamData->lin->pszNick);
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, " ");
	}

	// Insert the message
	{
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", Log_SetStyle(2, i));
		AddEventToBuffer(&buffer, &bufferEnd, &bufferAlloced, streamData);
	}

	//END RTF
	Log_Append(&buffer, &bufferEnd, &bufferAlloced, "}");
	return buffer;
}
static DWORD CALLBACK Log_StreamCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
    LOGSTREAMDATA *lstrdat = (LOGSTREAMDATA *) dwCookie;
 
	if (lstrdat->buffer == NULL) 
	{
		lstrdat->iColCount = 4;
        lstrdat->bufferOffset = 0;
		lstrdat->buffer = Log_CreateBody(lstrdat);
        lstrdat->bufferLen = lstrlen(lstrdat->buffer);
    }
    *pcb = min(cb, lstrdat->bufferLen - lstrdat->bufferOffset);
    CopyMemory(pbBuff, lstrdat->buffer + lstrdat->bufferOffset, *pcb);
    lstrdat->bufferOffset += *pcb;
    if (lstrdat->bufferOffset == lstrdat->bufferLen) 
	{
	    free(lstrdat->buffer);
        lstrdat->buffer = NULL;
    }
	
    return 0;
}
static BOOL LogToFile(LOGSTREAMDATA * streamData)
{
    char *buffer = 0;
    int bufferAlloced, bufferEnd;
	FILE *hFile = NULL;
	char szFile[MAX_PATH];
	char * pszModName = MM_FindModule(streamData->data->pszModule)->pszModDispName;
	char szName[MAX_PATH];
	char szFolder[MAX_PATH];

    bufferEnd = 0;
    bufferAlloced = 1024;
    buffer = (char *) malloc(bufferAlloced);
    buffer[0] = '\0';

	_snprintf(szName, MAX_PATH,"%s",pszModName?pszModName:streamData->data->pszModule);
	ValidateFilename(szName);
	_snprintf(szFolder, MAX_PATH,"%s\\%s", g_LogOptions.pszLogDir, szName );

	if(!PathIsDirectory(szFolder))
		CreateDirectory(szFolder, NULL);

	_snprintf(szName, MAX_PATH,"%s.log",streamData->data->pszID);
	ValidateFilename(szName);

	_snprintf(szFile, MAX_PATH,"%s\\%s", szFolder, szName ); 


	hFile = fopen(szFile,"a+");
	if(hFile)
	{
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "%s ", MakeTimeStamp(g_LogOptions.pszTimeStampLog, streamData->lin->time));
		if(streamData->lin->pszNick && streamData->lin->iType == GC_EVENT_MESSAGE)
		{
			char pszNick[30];
			char * p1;

			if(streamData->lin->bIsMe)
				lstrcpyn(pszNick, g_LogOptions.pszOutgoingNick, 29);
			else 
				lstrcpyn(pszNick, g_LogOptions.pszIncomingNick, 29);
			p1 = strchr(pszNick, '%');
			if(p1 && p1[1] == 'n')
				p1[1] = 's';
			
			Log_AppendRTF(streamData, &buffer, &bufferEnd, &bufferAlloced, pszNick, streamData->lin->pszNick);
			Log_Append(&buffer, &bufferEnd, &bufferAlloced, " ");
		}
		else
			Log_Append(&buffer, &bufferEnd, &bufferAlloced, "** ");

		AddEventToBuffer(&buffer, &bufferEnd, &bufferAlloced, streamData);	
		Log_Append(&buffer, &bufferEnd, &bufferAlloced, "\n");
		if(buffer)
			fputs(buffer, hFile);
		if(g_LogOptions.LoggingLimit > 0)
		{
			DWORD dwSize;
			DWORD trimlimit;

			fseek(hFile,0,SEEK_END);
			dwSize = ftell(hFile);
			rewind (hFile);
			trimlimit = g_LogOptions.LoggingLimit*1024+ 1024*10;
			if (dwSize > trimlimit)
			{
				BYTE * pBuffer = 0;
				int read = 0;

				pBuffer = (BYTE *)malloc(g_LogOptions.LoggingLimit*1024);
				fseek(hFile,-g_LogOptions.LoggingLimit*1024,SEEK_END);
				read = fread(pBuffer, 1, g_LogOptions.LoggingLimit*1024, hFile);
				fclose(hFile); 
				hFile = NULL;
				hFile = fopen(szFile,"wb");
				if(hFile && pBuffer && read)
				{
					fwrite(pBuffer, 1, read, hFile);
					fclose(hFile); hFile = NULL;
				}
				if(pBuffer)
					free(pBuffer);
			}
		}
		if (hFile)
			fclose(hFile); hFile = NULL;
		if(buffer)
			free(buffer);
		return TRUE;
	}
	if(buffer)
		free(buffer);
	return FALSE;
}
void Log_StreamInEvent(HWND hwndDlg,  LOGINFO* lin, CHATWINDOWDATA* dat, BOOL bRedraw)
{
	EDITSTREAM stream;
	LOGSTREAMDATA streamData;
	CHARRANGE oldsel, sel, newsel;
	POINT point ={0};
	SCROLLINFO scroll;
	WPARAM wp;
	HWND hwndRich;

	if(hwndDlg == 0 || lin == 0 || dat == 0)
		return;

	hwndRich = GetDlgItem(hwndDlg, IDC_LOG);
	ZeroMemory(&streamData, sizeof(LOGSTREAMDATA));
	streamData.hwnd = hwndRich;
	streamData.data = dat;
	streamData.lin = lin;
	streamData.bStripFormat = TRUE;
	
	if(!bRedraw && g_LogOptions.LoggingEnabled)
		LogToFile(&streamData);

	if(!IsWindowVisible(hwndDlg))
		return;

	streamData.bStripFormat = FALSE;

	if(bRedraw || dat->iType != GCW_CHATROOM || !dat->bFilterEnabled || (dat->iLogFilterFlags&lin->iType) != 0)
	{
		BOOL bFlag = FALSE;

		ZeroMemory(&stream, sizeof(stream));
		stream.pfnCallback = Log_StreamCallback;
		stream.dwCookie = (DWORD) & streamData;
		scroll.cbSize= sizeof(SCROLLINFO);
		scroll.fMask= SIF_RANGE | SIF_POS|SIF_PAGE;
		GetScrollInfo(GetDlgItem(hwndDlg, IDC_LOG), SB_VERT, &scroll);
		SendMessage(hwndRich, EM_GETSCROLLPOS, 0, (LPARAM) &point);

		SendMessage(hwndRich, EM_EXGETSEL, 0, (LPARAM) &oldsel);
		if (oldsel.cpMax != oldsel.cpMin)
		{
			SendMessage(hwndRich, WM_SETREDRAW, FALSE, 0);
		}

		sel.cpMin = sel.cpMax = GetRichTextLength(hwndRich);
		SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM) & sel);


		if(sel.cpMax == 0) // fix for the indent... must be a M$ bug
			bRedraw = TRUE;

		wp = bRedraw?SF_RTF:SFF_SELECTION|SF_RTF;

		if(bRedraw && IsWindowVisible(hwndRich))
		{
			SendMessage(hwndRich, WM_SETREDRAW, FALSE, 0);
			bFlag = TRUE;
		}

		if(bRedraw)
			SendMessage(hwndRich, WM_SETTEXT, 0, (LPARAM) "");

		for( ; ; ) 
		{
			if(dat->iType != GCW_CHATROOM || !dat->bFilterEnabled || (dat->iLogFilterFlags&lin->iType) != 0)
			{
				streamData.lin = lin;
				SendMessage(hwndRich, EM_STREAMIN, wp, (LPARAM) & stream);
				wp = SF_RTF|SFF_SELECTION;
			}
			if(lin->prev != NULL)
				lin = lin->prev;
			else
				break;
			sel.cpMin = sel.cpMax = GetRichTextLength(hwndRich);
			SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM) & sel);
		} 

		SendMessage(hwndRich, EM_EXGETSEL, (WPARAM)0, (LPARAM)&newsel);

		if (SmileyAddInstalled && (bRedraw || (lin->pszText 
			             && lin->iType != GC_EVENT_JOIN
			             && lin->iType != GC_EVENT_NICK
			             && lin->iType != GC_EVENT_ADDSTATUS
			             && lin->iType != GC_EVENT_REMOVESTATUS
						 ))) //smileys
		{
			SMADD_RICHEDIT2 sm;


			newsel.cpMin = newsel.cpMax - lstrlen(lin->pszText) - 10;
			if(newsel.cpMin < 0)
				newsel.cpMin = 0;
			ZeroMemory(&sm, sizeof(sm));
			sm.cbSize = sizeof(sm);
			sm.hwndRichEditControl = hwndRich;
			sm.Protocolname = dat->pszModule;
			sm.rangeToReplace = bRedraw?NULL:&newsel;
			sm.disableRedraw = TRUE;
			sm.useSounds = FALSE;
			CallService(MS_SMILEYADD_REPLACESMILEYS, 0, (LPARAM)&sm);
		}


		if (  (UINT)scroll.nPos >= (UINT)scroll.nMax-scroll.nPage-5 || scroll.nMax-scroll.nMin-scroll.nPage < 50)
		{ 
			newsel.cpMin = newsel.cpMax;
			SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM) & newsel);

			GetScrollInfo(hwndRich, SB_VERT, &scroll);
			scroll.fMask = SIF_POS;
			scroll.nPos = scroll.nMax - scroll.nPage + 1;
			SetScrollInfo(hwndRich, SB_VERT, &scroll, TRUE);
			SendMessage(hwndRich, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
		}
		else 
			SendMessage(hwndRich, EM_SETSCROLLPOS, 0, (LPARAM) &point);

		if (oldsel.cpMax != oldsel.cpMin)
		{
			SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM) & oldsel);
			SendMessage(hwndRich, WM_SETREDRAW, TRUE, 0);
			InvalidateRect(hwndRich, NULL, TRUE);
		}
		if(bFlag)
		{
			sel.cpMin = sel.cpMax = GetRichTextLength(hwndRich);
			SendMessage(hwndRich, EM_EXSETSEL, 0, (LPARAM) & sel);
			SendMessage(hwndRich, WM_SETREDRAW, TRUE, 0);
			InvalidateRect(hwndRich, NULL, TRUE);
		}
		

	}

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

	hBkgBrush = CreateSolidBrush(DBGetContactSettingDword(NULL, "Chat", "ColorLogBG", GetSysColor(COLOR_WINDOW)));
	bih.biSize = sizeof(bih);
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	bih.biHeight = 10; //GetSystemMetrics(SM_CYSMICON);
	bih.biPlanes = 1;
	bih.biWidth = 10; //GetSystemMetrics(SM_CXSMICON);
	widthBytes = ((bih.biWidth * bih.biBitCount + 31) >> 5) * 4;
	rc.top = rc.left = 0;
	rc.right = bih.biWidth;
	rc.bottom = bih.biHeight;
	hdc = GetDC(NULL);
	hBmp = CreateCompatibleBitmap(hdc, bih.biWidth, bih.biHeight);
	hdcMem = CreateCompatibleDC(hdc);
	pBmpBits = (PBYTE) malloc(widthBytes * bih.biHeight);
	for (i = 0; i < sizeof(pLogIconBmpBits) / sizeof(pLogIconBmpBits[0]); i++) {
		hIcon = hIcons[i+11];
		pLogIconBmpBits[i] = (PBYTE) malloc(RTFPICTHEADERMAXSIZE + (bih.biSize + widthBytes * bih.biHeight) * 2);
		rtfHeaderSize = sprintf(pLogIconBmpBits[i], "{\\pict\\dibitmap0\\wbmbitspixel%u\\wbmplanes1\\wbmwidthbytes%u\\picw%u\\pich%u ", bih.biBitCount, widthBytes, bih.biWidth, bih.biHeight);
		hoBmp = (HBITMAP) SelectObject(hdcMem, hBmp);
		FillRect(hdcMem, &rc, hBkgBrush);
		DrawIconEx(hdcMem, 0, 0, hIcon, bih.biWidth, bih.biHeight, 0, NULL, DI_NORMAL);
		SelectObject(hdcMem, hoBmp);
		GetDIBits(hdc, hBmp, 0, bih.biHeight, pBmpBits, (BITMAPINFO *) & bih, DIB_RGB_COLORS);
		{
			int n;
			for (n = 0; n < sizeof(BITMAPINFOHEADER); n++)
				sprintf(pLogIconBmpBits[i] + rtfHeaderSize + n * 2, "%02X", ((PBYTE) & bih)[n]);
			for (n = 0; n < widthBytes * bih.biHeight; n += 4)
				sprintf(pLogIconBmpBits[i] + rtfHeaderSize + (bih.biSize + n) * 2, "%02X%02X%02X%02X", pBmpBits[n], pBmpBits[n + 1], pBmpBits[n + 2], pBmpBits[n + 3]);
		}
		logIconBmpSize[i] = rtfHeaderSize + (bih.biSize + widthBytes * bih.biHeight) * 2 + 1;
		pLogIconBmpBits[i][logIconBmpSize[i] - 1] = '}';
	}
	free(pBmpBits);
	DeleteDC(hdcMem);
	DeleteObject(hBmp);
	ReleaseDC(NULL, hdc);
	DeleteObject(hBkgBrush);
}

void FreeMsgLogBitmaps(void)
{
    int i;
    for (i = 0; i < sizeof(pLogIconBmpBits) / sizeof(pLogIconBmpBits[0]); i++)
        free(pLogIconBmpBits[i]);
}
