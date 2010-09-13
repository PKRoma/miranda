/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project, 
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
*/

#include "commonheaders.h"


//KB167296
void UnixTimeToFileTime(time_t ts, LPFILETIME pft)
{
	unsigned __int64 ll = UInt32x32To64(ts, 10000000) + 116444736000000000i64;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

time_t FileTimeToUnixTime(LPFILETIME pft)
{
	unsigned __int64 ll = (unsigned __int64)pft->dwHighDateTime << 32 | pft->dwLowDateTime;
	ll -= 116444736000000000i64;
	return (time_t)(ll / 10000000);
}

static INT_PTR TimestampToLocal(WPARAM wParam,LPARAM)
{
	FILETIME ft, lft;

	UnixTimeToFileTime((time_t)wParam, &ft);
	FileTimeToLocalFileTime(&ft, &lft);

	return FileTimeToUnixTime(&lft);
}

void FormatTime(const SYSTEMTIME *st, const TCHAR *szFormat, TCHAR *szDest, int cbDest)
{
	if (szDest == NULL || cbDest == 0) return;

	TCHAR *pDest = szDest;
	int destCharsLeft = cbDest - 1;

	for (const TCHAR* pFormat = szFormat; *pFormat; ++pFormat) 
	{
		DWORD fmt;
		bool date;
		switch (*pFormat) 
		{
		case 't':
			fmt = TIME_NOSECONDS;
			date = false;
			break;

		case 's':
			fmt = 0;
			date = false;
			break;

		case 'm':
			fmt = TIME_NOMINUTESORSECONDS;
			date = false;
			break;

		case 'd':
			fmt = DATE_SHORTDATE;
			date = true;
			break;

		case 'D':
			fmt = DATE_LONGDATE;
			date = true;
			break;

		default:
			if (destCharsLeft--) 
				*pDest++ = *pFormat;
			continue;
		}
		
		TCHAR dateTimeStr[64];
		int dateTimeStrLen;

		if (date) 
			dateTimeStrLen = GetDateFormat(LOCALE_USER_DEFAULT, fmt, st, NULL, dateTimeStr, SIZEOF(dateTimeStr));
		else
			dateTimeStrLen = GetTimeFormat(LOCALE_USER_DEFAULT, fmt, st, NULL, dateTimeStr, SIZEOF(dateTimeStr));

		if (dateTimeStrLen) --dateTimeStrLen;
		if (destCharsLeft < dateTimeStrLen) dateTimeStrLen = destCharsLeft;
		memcpy(pDest, dateTimeStr, dateTimeStrLen * sizeof(dateTimeStr[0]));
		destCharsLeft -= dateTimeStrLen;
		pDest += dateTimeStrLen;
	}
	*pDest = 0;
}

#ifdef _UNICODE
void FormatTimeA(const SYSTEMTIME *st, const char *szFormat, char *szDest, int cbDest)
{
	if (szDest == NULL || cbDest == 0) return;

	char *pDest = szDest;
	int destCharsLeft = cbDest - 1;

	for (const char* pFormat = szFormat; *pFormat; ++pFormat) 
	{
		DWORD fmt;
		bool date;
		switch (*pFormat) 
		{
		case 't':
			fmt = TIME_NOSECONDS;
			date = false;
			break;

		case 's':
			fmt = 0;
			date = false;
			break;

		case 'm':
			fmt = TIME_NOMINUTESORSECONDS;
			date = false;
			break;

		case 'd':
			fmt = DATE_SHORTDATE;
			date = true;
			break;

		case 'D':
			fmt = DATE_LONGDATE;
			date = true;
			break;

		default:
			if (destCharsLeft--) 
				*pDest++ = *pFormat;
			continue;
		}
		
		char dateTimeStr[64];
		int dateTimeStrLen;

		if (date) 
			dateTimeStrLen = GetDateFormatA(LOCALE_USER_DEFAULT, fmt, st, NULL, dateTimeStr, SIZEOF(dateTimeStr));
		else
			dateTimeStrLen = GetTimeFormatA(LOCALE_USER_DEFAULT, fmt, st, NULL, dateTimeStr, SIZEOF(dateTimeStr));

		if (dateTimeStrLen) --dateTimeStrLen;
		if (destCharsLeft < dateTimeStrLen) dateTimeStrLen = destCharsLeft;
		memcpy(pDest, dateTimeStr, dateTimeStrLen * sizeof(dateTimeStr[0]));
		destCharsLeft -= dateTimeStrLen;
		pDest += dateTimeStrLen;
	}
	*pDest = 0;
}
#endif

void TimeStampToSystemTime(time_t ts, SYSTEMTIME *st)
{
	FILETIME ft, lft;

	UnixTimeToFileTime(ts, &ft);
	FileTimeToLocalFileTime(&ft, &lft);
	FileTimeToSystemTime(&lft, st);
}

static INT_PTR TimestampToString(WPARAM wParam, LPARAM lParam)
{
	DBTIMETOSTRING *tts = (DBTIMETOSTRING*)lParam;
	if (tts == NULL) return 0;

	SYSTEMTIME st;
	TimeStampToSystemTime((time_t)wParam, &st);

#ifdef _UNICODE
	FormatTimeA(&st, tts->szFormat, tts->szDest, tts->cbDest);
#else
	FormatTime(&st, tts->szFormat, tts->szDest, tts->cbDest); 
#endif
	return 0;
}

#ifdef _UNICODE
static INT_PTR TimestampToStringW(WPARAM wParam, LPARAM lParam)
{
	DBTIMETOSTRINGT *tts = (DBTIMETOSTRINGT*)lParam;
	if (tts == NULL) return 0;

	SYSTEMTIME st;
	TimeStampToSystemTime((time_t)wParam, &st);
	FormatTime(&st, tts->szFormat, tts->szDest, tts->cbDest); 
	return 0;
}
#endif

int InitTime(void)
{
	CreateServiceFunction(MS_DB_TIME_TIMESTAMPTOLOCAL, TimestampToLocal);
	CreateServiceFunction(MS_DB_TIME_TIMESTAMPTOSTRING, TimestampToString);
	#if defined( _UNICODE )
		CreateServiceFunction(MS_DB_TIME_TIMESTAMPTOSTRINGT, TimestampToStringW);
	#else
		CreateServiceFunction(MS_DB_TIME_TIMESTAMPTOSTRINGT, TimestampToString);
	#endif
	return 0;
} 
