/*
Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2010 Miranda ICQ/IM project,
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

implements services to handle location - based timezones, instead of
simple GMT offsets.
*/

#include <commonheaders.h>

TIME_API tmi;

#if _MSC_VER < 1500
	typedef struct _TIME_DYNAMIC_ZONE_INFORMATION_T {
		LONG Bias;
		WCHAR StandardName[ 32 ];
		SYSTEMTIME StandardDate;
		LONG StandardBias;
		WCHAR DaylightName[ 32 ];
		SYSTEMTIME DaylightDate;
		LONG DaylightBias;
		WCHAR TimeZoneKeyName[ 128 ];
		BOOLEAN DynamicDaylightTimeDisabled;
	} DYNAMIC_TIME_ZONE_INFORMATION;
#endif

typedef DWORD 	(WINAPI *pfnGetDynamicTimeZoneInformation_t)(DYNAMIC_TIME_ZONE_INFORMATION *pdtzi);
static pfnGetDynamicTimeZoneInformation_t pfnGetDynamicTimeZoneInformation;

typedef HRESULT	(WINAPI *pfnSHLoadIndirectString_t)(LPCWSTR pszSource, LPWSTR pszOutBuf, UINT cchOutBuf,  void **ppvReserved);
static pfnSHLoadIndirectString_t pfnSHLoadIndirectString;

typedef LANGID (WINAPI *pfnGetUserDefaultUILanguage_t)(void);
static pfnGetUserDefaultUILanguage_t pfnGetUserDefaultUILanguage;

typedef LANGID (WINAPI *pfnGetSystemDefaultUILanguage_t)(void);
static pfnGetSystemDefaultUILanguage_t pfnGetSystemDefaultUILanguage;


typedef struct _REG_TZI_FORMAT
{
    LONG Bias;
    LONG StandardBias;
    LONG DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
} REG_TZI_FORMAT;

#define MIM_TZ_DISPLAYLEN 128

struct MIM_TIMEZONE
{
	TCHAR	tszName[MIM_TZ_NAMELEN];			// windows name for the time zone
	TCHAR	tszDisplay[MIM_TZ_DISPLAYLEN];		// more descriptive display name (that's what usually appears in dialogs)
												// every hour should be sufficient.
	TIME_ZONE_INFORMATION tzi;
	unsigned hash;
	int offset;

	static int compareHash(const MIM_TIMEZONE* p1, const MIM_TIMEZONE* p2)
	{ return p1->hash - p2->hash; }

	static int compareBias(const MIM_TIMEZONE* p1, const MIM_TIMEZONE* p2)
	{ return p2->tzi.Bias - p1->tzi.Bias; }
};

typedef struct
{
	TIME_ZONE_INFORMATION tzi;
	DWORD		timestamp;					// last time updated
	MIM_TIMEZONE *myTZ;						// set to my own timezone
} TZ_INT_INFO;

static TZ_INT_INFO myInfo;
bool muiInstalled;

static OBJLIST<MIM_TIMEZONE>  g_timezones(55, MIM_TIMEZONE::compareHash);
static LIST<MIM_TIMEZONE>     g_timezonesBias(55, MIM_TIMEZONE::compareBias);

void FormatTime (const SYSTEMTIME *st, const TCHAR *szFormat, TCHAR *szDest, int cbDest);
void UnixTimeToFileTime(time_t ts, LPFILETIME pft);
time_t FileTimeToUnixTime(LPFILETIME pft);

#ifdef _UNICODE 
#define fnSystemTimeToTzSpecificLocalTime SystemTimeToTzSpecificLocalTime
#else
BOOL MySystemTimeToTzSpecificLocalTime(const TIME_ZONE_INFORMATION *ptzi, const SYSTEMTIME *pstUtc, SYSTEMTIME *pstLoc);
#define fnSystemTimeToTzSpecificLocalTime MySystemTimeToTzSpecificLocalTime
#endif


static int timeapiGetTimeZoneTime(HANDLE hTZ, SYSTEMTIME *st)
{
	if (st == NULL) return 1;


	MIM_TIMEZONE *tz = (MIM_TIMEZONE*)hTZ;
	if (tz && tz != myInfo.myTZ)
	{
		SYSTEMTIME sto;
		GetSystemTime(&sto);
		return !fnSystemTimeToTzSpecificLocalTime(&tz->tzi, &sto, st);
	}
	else
		GetLocalTime(st);

	return 0;
}

static void CalcTsOffset(MIM_TIMEZONE *tz)
{
	SYSTEMTIME st, stl;
	GetSystemTime(&st);

	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);
	time_t ts1 = FileTimeToUnixTime(&ft);

	if (!fnSystemTimeToTzSpecificLocalTime(&tz->tzi, &st, &stl))
		return;

	SystemTimeToFileTime(&stl, &ft);
	time_t ts2 = FileTimeToUnixTime(&ft);

	tz->offset = ts2 - ts1;
}

static bool IsSameTime(MIM_TIMEZONE *tz)
{
	SYSTEMTIME st, stl;

	if (tz == myInfo.myTZ) 
		return true;

	timeapiGetTimeZoneTime(tz, &stl);
	timeapiGetTimeZoneTime(NULL, &st);

	return st.wHour == stl.wHour && st.wMinute == stl.wMinute;
}

static HANDLE timeapiGetInfoByName(LPCTSTR tszName, DWORD dwFlags)
{
	if (tszName == NULL)
		return (dwFlags & (TZF_DIFONLY | TZF_KNOWNONLY)) ? NULL : myInfo.myTZ;

	MIM_TIMEZONE tzsearch;
	tzsearch.hash = hashstr(tszName);

	MIM_TIMEZONE *tz = g_timezones.find(&tzsearch);
	if (tz == NULL)
		return (dwFlags & (TZF_DIFONLY | TZF_KNOWNONLY)) ? NULL : myInfo.myTZ;

	if (dwFlags & TZF_DIFONLY)
		return IsSameTime(tz) ? NULL : tz;

	return tz;
}

static HANDLE timeapiGetInfoByContact(HANDLE hContact, DWORD dwFlags)
{
	if (hContact == NULL)
		return (dwFlags & (TZF_DIFONLY | TZF_KNOWNONLY)) ? NULL : myInfo.myTZ;

	DBVARIANT	dbv;
	if (!DBGetContactSettingTString(hContact, "UserInfo", "TzName", &dbv)) 
	{
		HANDLE res = timeapiGetInfoByName(dbv.ptszVal, dwFlags);  
		DBFreeVariant(&dbv);
		return res;
	} 
	else 
	{
		signed char timezone = (signed char)DBGetContactSettingByte(hContact, "UserInfo", "Timezone", -1);
		if (timezone == -1)
		{
			char* szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
			timezone = (signed char)DBGetContactSettingByte(hContact, szProto, "Timezone", -1);
		}

		if (timezone != -1) 
		{
			MIM_TIMEZONE tzsearch;
			tzsearch.tzi.Bias = timezone * 30;
			int i = g_timezonesBias.getIndex(&tzsearch);
			
			while (i >= 0 && g_timezonesBias[i]->tzi.Bias == tzsearch.tzi.Bias) --i; 
			
			int delta = LONG_MAX;
			for (int j = ++i; j < g_timezonesBias.getCount() && g_timezonesBias[j]->tzi.Bias == tzsearch.tzi.Bias; ++j)
			{
				int delta1 = abs(g_timezonesBias[j]->tzi.DaylightDate.wMonth - myInfo.tzi.DaylightDate.wMonth);
				if (delta1 <= delta)
				{
					delta = delta1;
					i = j;
				}
			}

			if (i >= 0)
			{
				MIM_TIMEZONE *tz = g_timezonesBias[i];
				if (dwFlags & TZF_DIFONLY)
					return IsSameTime(tz) ? NULL : tz;

				return tz;
			}
		}
		return (dwFlags & (TZF_DIFONLY | TZF_KNOWNONLY)) ? NULL : myInfo.myTZ;
	}
}

static void timeapiSetInfoByContact(HANDLE hContact, HANDLE hTZ)
{
	MIM_TIMEZONE *tz = (MIM_TIMEZONE*)hTZ;

	if (hContact == NULL) return;

	if (tz)
	{
		DBWriteContactSettingTString(hContact, "UserInfo", "TzName", tz->tszName);
		DBWriteContactSettingByte(hContact, "UserInfo", "Timezone", (char)((tz->tzi.Bias + tz->tzi.StandardBias) / 30));
	} 
	else 
	{
		DBDeleteContactSetting(hContact, "UserInfo", "Timezone");
		DBDeleteContactSetting(hContact, "UserInfo", "TzName");
	}
}

static int timeapiPrintDateTime(HANDLE hTZ, LPCTSTR szFormat, LPTSTR szDest, int cbDest, DWORD dwFlags)
{
	MIM_TIMEZONE *tz = (MIM_TIMEZONE*)hTZ;
	if (tz == NULL && (dwFlags & (TZF_DIFONLY | TZF_KNOWNONLY))) 
		return 1;

	SYSTEMTIME st;
	if (timeapiGetTimeZoneTime(tz, &st))
		return 1;

	FormatTime(&st, szFormat, szDest, cbDest);

	return 0;
}

static int timeapiPrintTimeStamp(HANDLE hTZ, time_t ts, LPCTSTR szFormat, LPTSTR szDest, int cbDest, DWORD dwFlags)
{
	MIM_TIMEZONE *tz = (MIM_TIMEZONE*)hTZ;
	if (tz == NULL && (dwFlags & (TZF_DIFONLY | TZF_KNOWNONLY))) 
		return 1;

	FILETIME ft;

	if (tz == NULL) tz = myInfo.myTZ; 
	if (tz == NULL)
	{
		FILETIME lft;

		UnixTimeToFileTime(ts, &lft);
		FileTimeToLocalFileTime(&lft, &ft);
	}
	else
	{
		if (tz->offset == INT_MIN)
			CalcTsOffset(tz);

		UnixTimeToFileTime(ts + tz->offset, &ft);
	}
	
	SYSTEMTIME st;
	FileTimeToSystemTime(&ft, &st);

	FormatTime(&st, szFormat, szDest, cbDest);

	return 0;
}

static LPTIME_ZONE_INFORMATION timeapiGetTzi(HANDLE hTZ)
{
	MIM_TIMEZONE *tz = (MIM_TIMEZONE*)hTZ;
	return tz ? &tz->tzi : &myInfo.tzi;
}


static time_t timeapiTimeStampToTimeZoneTimeStamp(HANDLE hTZ, time_t ts)
{
	MIM_TIMEZONE *tz = (MIM_TIMEZONE*)hTZ;
	
	if (tz == NULL) tz = myInfo.myTZ; 
	if (tz == NULL)
	{
		FILETIME ft, lft;

		UnixTimeToFileTime(ts, &ft);
		FileTimeToLocalFileTime(&ft, &lft);
		return FileTimeToUnixTime(&lft);
	}
	
	if (tz->offset == INT_MIN)
		CalcTsOffset(tz);

	return ts + tz->offset;
}

typedef struct
{
	UINT addStr, getSel, setSel, getData, setData;
} ListMessages;

static const ListMessages lbMessages =
{ LB_ADDSTRING, LB_GETCURSEL, LB_SETCURSEL, LB_GETITEMDATA, LB_SETITEMDATA };

static const ListMessages cbMessages =
{ CB_ADDSTRING, CB_GETCURSEL, CB_SETCURSEL, CB_GETITEMDATA, CB_SETITEMDATA };

static const ListMessages *GetListMessages(HWND hWnd, DWORD dwFlags)
{
	if (!(dwFlags & (TZF_PLF_CB | TZF_PLF_LB))) 
	{
		TCHAR	tszClassName[128];
		GetClassName(hWnd, tszClassName, SIZEOF(tszClassName));
		if (!_tcsicmp(tszClassName, _T("COMBOBOX")))
			dwFlags |= TZF_PLF_CB;
		else if(!_tcsicmp(tszClassName, _T("LISTBOX")))
			dwFlags |= TZF_PLF_LB;
	}
	if (dwFlags & TZF_PLF_CB) 
		return & cbMessages;
	else if(dwFlags & TZF_PLF_LB) 
		return & lbMessages;
	else
		return NULL;
}


static int timeapiSelectListItem(HANDLE hContact, HWND hWnd, DWORD dwFlags)
{
	if (hWnd == NULL || hContact == NULL)	   // nothing to do
		return -1;

	const ListMessages *lstMsg = GetListMessages(hWnd, dwFlags);
	if (lstMsg == NULL) return -1;

	int iSelection = 0;
	DBVARIANT dbv;
	if (!DBGetContactSettingTString(hContact, "UserInfo", "TzName", &dbv)) 
	{
		MIM_TIMEZONE tzsearch;
		tzsearch.hash = hashstr(dbv.ptszVal);
		iSelection = g_timezones.getIndex(&tzsearch) + 1;
		DBFreeVariant(&dbv);
	}

	SendMessage(hWnd, lstMsg->setSel, iSelection, 0);
	return iSelection;
}


static int timeapiPrepareList(HANDLE hContact, HWND hWnd, DWORD dwFlags)
{
	if (hWnd == NULL)	   // nothing to do
		return 0;

	const ListMessages *lstMsg = GetListMessages(hWnd, dwFlags);
	if (lstMsg == NULL) return 0;

	SendMessage(hWnd, lstMsg->addStr, 0, (LPARAM)TranslateT("<unspecified>"));

	for (int i = 0; i < g_timezonesBias.getCount(); ++i) 
	{
		MIM_TIMEZONE *tz = g_timezonesBias[i];

		SendMessage(hWnd, lstMsg->addStr, 0, (LPARAM)tz->tszDisplay);
		SendMessage(hWnd, lstMsg->setData, i + 1, (LPARAM)tz);
	}

	return timeapiSelectListItem(hContact, hWnd, dwFlags);
}


static void timeapiStoreListResult(HANDLE hContact, HWND hWnd, DWORD dwFlags)
{
	const ListMessages *lstMsg = GetListMessages(hWnd, dwFlags);
	if (lstMsg == NULL) return;

	LRESULT offset = SendMessage(hWnd, lstMsg->getSel, 0, 0);
	if (offset > 0) 
	{
		MIM_TIMEZONE *tz = (MIM_TIMEZONE*)SendMessage(hWnd, lstMsg->getData, offset, 0);
		if ((INT_PTR)tz != CB_ERR && tz != NULL)
			timeapiSetInfoByContact(hContact, tz);
	} 
	else
		timeapiSetInfoByContact(hContact, NULL);
}


static INT_PTR GetTimeApi( WPARAM, LPARAM lParam )
{
	TIME_API* tmi = (TIME_API*)lParam;
	if (tmi == NULL)
		return FALSE;

	if (tmi->cbSize != sizeof(TIME_API))
		return FALSE;

	tmi->createByName                    = timeapiGetInfoByName;
	tmi->createByContact                 = timeapiGetInfoByContact;
	tmi->storeByContact                  = timeapiSetInfoByContact;

	tmi->printDateTime                   = timeapiPrintDateTime;
	tmi->printTimeStamp                  = timeapiPrintTimeStamp;

	tmi->prepareList                     = timeapiPrepareList;
	tmi->selectListItem                  = timeapiSelectListItem;
	tmi->storeListResults                = timeapiStoreListResult;

	tmi->getTzi                          = timeapiGetTzi;
	tmi->getTimeZoneTime                 = timeapiGetTimeZoneTime;
	tmi->timeStampToTimeZoneTimeStamp    = timeapiTimeStampToTimeZoneTimeStamp;

	return TRUE;
}

static INT_PTR TimestampToLocal(WPARAM wParam, LPARAM)
{
	return timeapiTimeStampToTimeZoneTimeStamp(NULL, (time_t)wParam);
}

static INT_PTR TimestampToStringT(WPARAM wParam, LPARAM lParam)
{
	DBTIMETOSTRINGT *tts = (DBTIMETOSTRINGT*)lParam;
	if (tts == NULL) return 0;

	timeapiPrintTimeStamp(NULL, (time_t)wParam, tts->szFormat, tts->szDest, tts->cbDest, 0); 
	return 0;
}

#ifdef _UNICODE
static INT_PTR TimestampToStringA(WPARAM wParam, LPARAM lParam)
{
	DBTIMETOSTRING *tts = (DBTIMETOSTRING*)lParam;
	if (tts == NULL) return 0;

	TCHAR *szDest = (TCHAR*)alloca(tts->cbDest);
	timeapiPrintTimeStamp(NULL, (time_t)wParam, StrConvT(tts->szFormat), szDest, tts->cbDest, 0); 
	WideCharToMultiByte(CP_ACP, 0, szDest, -1, tts->szDest, tts->cbDest, NULL, NULL);
	return 0;
}
#endif

void GetLocalizedString(HKEY hSubKey, const TCHAR *szName, TCHAR *szBuf, DWORD cbLen)
{
	szBuf[0] = 0;
#ifdef _UNICODE
	if (muiInstalled)
	{
		TCHAR tszTempBuf[MIM_TZ_NAMELEN], tszName[30];
		mir_sntprintf(tszName, SIZEOF(tszName), _T("MUI_%s"), szName);
		DWORD dwLength = cbLen * sizeof(TCHAR);
		if (ERROR_SUCCESS == RegQueryValueEx(hSubKey, tszName, NULL, NULL, (unsigned char *)tszTempBuf, &dwLength))
			pfnSHLoadIndirectString(tszTempBuf, szBuf, cbLen, NULL);
	}
	if (szBuf[0] == 0)
#endif
	{
		DWORD dwLength = cbLen * sizeof(TCHAR);
		RegQueryValueEx(hSubKey, szName, NULL, NULL, (unsigned char *)szBuf, &dwLength);
	}
}

void InitTimeZones(void)
{
	GetTimeZoneInformation(&myInfo.tzi);
	myInfo.timestamp = time(NULL);

	REG_TZI_FORMAT	tzi;
	HKEY			hKey;
	TCHAR           *myTzKey = NULL;
	DYNAMIC_TIME_ZONE_INFORMATION dtzi;

	const TCHAR *tszKey = IsWinVerNT() ?
		_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones") :
		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Time Zones");

	/*
	 * use GetDynamicTimeZoneInformation() on Vista+ - this will return a structure with
	 * the registry key name, so finding our own time zone later will be MUCH easier for
	 * localized systems or systems with a MUI pack installed
	 */
	if (IsWinVerVistaPlus()) 
	{
		pfnGetDynamicTimeZoneInformation = (pfnGetDynamicTimeZoneInformation_t)GetProcAddress(GetModuleHandle(_T("kernel32")), "GetDynamicTimeZoneInformation");
		if (pfnGetDynamicTimeZoneInformation)
			if (pfnGetDynamicTimeZoneInformation(&dtzi) != TIME_ZONE_ID_INVALID)
				myTzKey = mir_u2t(dtzi.TimeZoneKeyName);
	}

#ifdef _UNICODE
	if (IsWinVer2000Plus())
	{
		pfnSHLoadIndirectString = (pfnSHLoadIndirectString_t)GetProcAddress(GetModuleHandle(_T("shlwapi")), "SHLoadIndirectString");
		pfnGetSystemDefaultUILanguage = (pfnGetSystemDefaultUILanguage_t)GetProcAddress(GetModuleHandle(_T("kernel32")), "GetSystemDefaultUILanguage");
		pfnGetUserDefaultUILanguage = (pfnGetUserDefaultUILanguage_t)GetProcAddress(GetModuleHandle(_T("kernel32")), "GetUserDefaultUILanguage");
		muiInstalled = pfnSHLoadIndirectString && pfnGetSystemDefaultUILanguage() != pfnGetUserDefaultUILanguage();
	}
#endif

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, tszKey, 0, KEY_ENUMERATE_SUB_KEYS, &hKey)) 
	{
		DWORD	dwIndex = 0;
		HKEY	hSubKey;
		TCHAR	tszName[MIM_TZ_NAMELEN];
		TCHAR	szStdName[MIM_TZ_NAMELEN], szDayName[MIM_TZ_NAMELEN];

		TCHAR* myStdName = mir_u2t(myInfo.tzi.StandardName);
		TCHAR* myDayName = mir_u2t(myInfo.tzi.DaylightName);

		DWORD dwSize = sizeof(tszName);
		while (ERROR_NO_MORE_ITEMS != RegEnumKeyEx(hKey, dwIndex++, tszName, &dwSize, NULL, NULL, 0, NULL))
		{
			tszName[MIM_TZ_NAMELEN -1 ] = 0;
			if (ERROR_SUCCESS == RegOpenKeyEx(hKey, tszName, 0, KEY_QUERY_VALUE, &hSubKey)) 
			{
				dwSize = sizeof(tszName);

				DWORD dwLength = sizeof(tzi);
				if (ERROR_SUCCESS != RegQueryValueEx(hSubKey, _T("TZI"), NULL, NULL, (unsigned char *)&tzi, &dwLength)) 
					continue;

				MIM_TIMEZONE *tz = new MIM_TIMEZONE;

				tz->tzi.Bias = tzi.Bias;
				tz->tzi.StandardDate = tzi.StandardDate;
				tz->tzi.StandardBias = tzi.StandardBias;
				tz->tzi.DaylightDate = tzi.DaylightDate;
				tz->tzi.DaylightBias = tzi.DaylightBias;

				_tcscpy(tz->tszName, tszName);
				tz->hash = hashstr(tszName);
				tz->offset = INT_MIN;

				GetLocalizedString(hSubKey, _T("Display"), tz->tszDisplay, SIZEOF(tz->tszDisplay));
				GetLocalizedString(hSubKey, _T("Std"), szStdName, SIZEOF(szStdName));
				GetLocalizedString(hSubKey, _T("Dlt"), szDayName, SIZEOF(szDayName));

#ifndef _UNICODE
				MultiByteToWideChar(CP_ACP, 0, szStdName, -1, tz->tzi.StandardName, SIZEOF(tz->tzi.StandardName));
				MultiByteToWideChar(CP_ACP, 0, szDayName, -1, tz->tzi.DaylightName, SIZEOF(tz->tzi.DaylightName));
#else
				lstrcpyn(tz->tzi.StandardName, szStdName, SIZEOF(tz->tzi.StandardName));
				lstrcpyn(tz->tzi.DaylightName, szDayName, SIZEOF(tz->tzi.DaylightName));
#endif
				if (myInfo.myTZ == NULL)
				{
					// compare registry key names (= not localized, so it's easy)
					if (myTzKey && !_tcscmp(tszName, myTzKey)) 
					{
						myInfo.myTZ = tz;
					}
					else 
					{
						if (!_tcscmp(szStdName, myStdName) || !_tcscmp(szDayName, myDayName)) 
							myInfo.myTZ = tz;
					}
				}

				g_timezones.insert(tz);
				g_timezonesBias.insert(tz);

				RegCloseKey(hSubKey);
			}
			dwSize = SIZEOF(tszName);
		}
		RegCloseKey(hKey);

		mir_free(myStdName);
		mir_free(myDayName);
	}
	mir_free(myTzKey);

	CreateServiceFunction(MS_SYSTEM_GET_TMI, GetTimeApi);

	CreateServiceFunction(MS_DB_TIME_TIMESTAMPTOLOCAL, TimestampToLocal);
	CreateServiceFunction(MS_DB_TIME_TIMESTAMPTOSTRINGT, TimestampToStringT);
#ifdef _UNICODE
	CreateServiceFunction(MS_DB_TIME_TIMESTAMPTOSTRING, TimestampToStringA);
#else
	CreateServiceFunction(MS_DB_TIME_TIMESTAMPTOSTRING, TimestampToStringT);
#endif


	tmi.cbSize = sizeof(tmi);
	GetTimeApi(0, (LPARAM)&tmi);
}

void UninitTimeZones(void)
{
	g_timezonesBias.destroy();
	g_timezones.destroy();
}
