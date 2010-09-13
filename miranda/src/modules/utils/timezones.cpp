/*
Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2010 Miranda ICQ/IM project,
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

typedef DWORD 		(WINAPI *pfnGetDynamicTimeZoneInformation_t)(DYNAMIC_TIME_ZONE_INFORMATION *pdtzi);
static pfnGetDynamicTimeZoneInformation_t pfnGetDynamicTimeZoneInformation;

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

	static int compareHash(const MIM_TIMEZONE* p1, const MIM_TIMEZONE* p2)
	{ return p1->hash - p2->hash; }

	static int compareBias(const MIM_TIMEZONE* p1, const MIM_TIMEZONE* p2)
	{ return p2->tzi.Bias - p1->tzi.Bias; }
};

/*
 * our own time zone information
 */
typedef struct
{
	MIM_TIMEZONE *tz;						// set to my own timezone
	DWORD	timestamp;	                    // last time updated
} TZ_HANDLE;


typedef struct
{
	TIME_ZONE_INFORMATION tzi;
	DWORD		timestamp;					// last time updated
	MIM_TIMEZONE *myTZ;						// set to my own timezone
} TZ_INT_INFO;

static TZ_INT_INFO myInfo;

static OBJLIST<MIM_TIMEZONE>  g_timezones(55, MIM_TIMEZONE::compareHash);
static LIST<MIM_TIMEZONE>     g_timezonesBias(55, MIM_TIMEZONE::compareBias);

void FormatTime (const SYSTEMTIME *st, const TCHAR *szFormat, TCHAR *szDest, int cbDest);
void FormatTimeA(const SYSTEMTIME *st, const char *szFormat, char *szDest, int cbDest);

static bool IsSameTime(TIME_ZONE_INFORMATION *tzi)
{
	SYSTEMTIME st, stl;
	GetSystemTime(&st);
	SystemTimeToTzSpecificLocalTime(tzi, &st, &stl);
	GetLocalTime(&st);
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
		return IsSameTime(&tz->tzi) ? NULL : tz;

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
					return IsSameTime(&tz->tzi) ? NULL : tz;

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
		DBWriteContactSettingByte(hContact, "UserInfo", "Timezone", (char)((tz->tzi.Bias + tz->tzi.StandardBias) / -60));
	} 
	else 
	{
		DBDeleteContactSetting(hContact, "UserInfo", "Timezone");
		DBDeleteContactSetting(hContact, "UserInfo", "TzName");
	}
}

#ifndef _UNICODE
void ConvertToAbsolute (const SYSTEMTIME * pstLoc, const SYSTEMTIME * pstDst, SYSTEMTIME * pstDstAbs)
{
     static int    iDays [12] = { 31, 28, 31, 30, 31, 30, 
                                  31, 31, 30, 31, 30, 31 } ;
     int           iDay ;

          // Set up the aboluste date structure except for wDay, which we must find

     pstDstAbs->wYear         = pstLoc->wYear ;      // Notice from local date/time
     pstDstAbs->wMonth        = pstDst->wMonth ;
     pstDstAbs->wDayOfWeek    = pstDst->wDayOfWeek ;

     pstDstAbs->wHour         = pstDst->wHour ;
     pstDstAbs->wMinute       = pstDst->wMinute ;
     pstDstAbs->wSecond       = pstDst->wSecond ;
     pstDstAbs->wMilliseconds = pstDst->wMilliseconds ;

          // Fix the iDays array for leap years

     if ((pstLoc->wYear % 4 == 0) && ((pstLoc->wYear % 100 != 0) || 
                                      (pstLoc->wYear % 400 == 0)))
     {
          iDays[1] = 29 ;
     }

          // Find a day of the month that falls on the same 
          //   day of the week as the transition.

          // Suppose today is the 20th of the month (pstLoc->wDay = 20)
          // Suppose today is a Wednesday (pstLoc->wDayOfWeek = 3)
          // Suppose the transition occurs on a Friday (pstDst->wDayOfWeek = 5)
          // Then iDay = 31, meaning that the 31st falls on a Friday
          // (The 7 is this formula avoids negatives.)

     iDay = pstLoc->wDay + pstDst->wDayOfWeek + 7 - pstLoc->wDayOfWeek ;

          // Now shrink iDay to a value between 1 and 7.

     iDay = (iDay - 1) % 7 + 1 ;

          // Now iDay is a day of the month ranging from 1 to 7.
          // Recall that the wDay field of the structure can range
          //   from 1 to 5, 1 meaning "first", 2 meaning "second",
          //   and 5 meaning "last".
          // So, increase iDay so it's the proper day of the month.

     iDay += 7 * (pstDst->wDay - 1) ;

          // Could be that iDay overshot the end of the month, so
          //   fix it up using the number of days in each month

     if (iDay > iDays[pstDst->wMonth - 1])
          iDay -= 7 ;

          // Assign that day to the structure. 

     pstDstAbs->wDay = iDay ;
}

BOOL LocalGreaterThanTransition (const SYSTEMTIME * pstLoc, const SYSTEMTIME * pstTran)
{
     FILETIME      ftLoc, ftTran ;
     LARGE_INTEGER liLoc, liTran ;
     SYSTEMTIME    stTranAbs ;

          // Easy case: Just compare the two months

     if (pstLoc->wMonth != pstTran->wMonth)
          return (pstLoc->wMonth > pstTran->wMonth) ;

          // Well, we're in a transition month. That requires a bit more work.

          // Check if pstDst is in absolute or day-in-month format.
          //   (See documentation of TIME_ZONE_INFORMATION, StandardDate field.)

     if (pstTran->wYear)       // absolute format (haven't seen one yet!)
     {
          stTranAbs = * pstTran ;
     }
     else                     // day-in-month format
     {
          ConvertToAbsolute (pstLoc, pstTran, &stTranAbs) ;
     }

          // Now convert both date/time structures to large integers & compare
     
     SystemTimeToFileTime (pstLoc, &ftLoc) ;
     liLoc = * (LARGE_INTEGER *) (void *) &ftLoc ;

     SystemTimeToFileTime (&stTranAbs, &ftTran) ;
     liTran = * (LARGE_INTEGER *) (void *) &ftTran ;

     return (liLoc.QuadPart > liTran.QuadPart) ;
}

BOOL MySystemTimeToTzSpecificLocalTime(const TIME_ZONE_INFORMATION *ptzi, const SYSTEMTIME *pstUtc, SYSTEMTIME *pstLoc) 
{
	// st is UTC

	FILETIME      ft ;
	LARGE_INTEGER li ;
	SYSTEMTIME    stDst ;

	// Convert time to a LARGE_INTEGER and subtract the bias

	SystemTimeToFileTime (pstUtc, &ft) ;
	li = * (LARGE_INTEGER *) (void *) &ft;
	li.QuadPart -= (LONGLONG) 600000000 * ptzi->Bias ;

	// Convert to a local date/time before application of daylight saving time.
	// The local date/time must be used to determine when the conversion occurs.

	ft = * (FILETIME *) (void *) &li ;
	FileTimeToSystemTime (&ft, pstLoc) ;

	// Find the time assuming Daylight Saving Time

	li.QuadPart -= (LONGLONG) 600000000 * ptzi->DaylightBias ;
	ft = * (FILETIME *) (void *) &li ;
	FileTimeToSystemTime (&ft, &stDst) ;

	// Now put li back the way it was

	li.QuadPart += (LONGLONG) 600000000 * ptzi->DaylightBias ;

	if (ptzi->StandardDate.wMonth)          // ie, daylight savings time
	{
          // Northern hemisphere
          if ((ptzi->DaylightDate.wMonth < ptzi->StandardDate.wMonth) &&

               (stDst.wMonth >= pstLoc->wMonth) &&           // avoid the end of year problem
               
               LocalGreaterThanTransition (pstLoc, &ptzi->DaylightDate) &&
              !LocalGreaterThanTransition (&stDst, &ptzi->StandardDate))
          {
               li.QuadPart -= (LONGLONG) 600000000 * ptzi->DaylightBias ;
          }
               // Southern hemisphere

          else if ((ptzi->StandardDate.wMonth < ptzi->DaylightDate.wMonth) &&
                  (!LocalGreaterThanTransition (&stDst, &ptzi->StandardDate) ||
                    LocalGreaterThanTransition (pstLoc, &ptzi->DaylightDate)))
          {
               li.QuadPart -= (LONGLONG) 600000000 * ptzi->DaylightBias ;
          }
          else
          {
               li.QuadPart -= (LONGLONG) 600000000 * ptzi->StandardBias ;
          }
     }

     ft = * (FILETIME *) (void *) &li ;
     FileTimeToSystemTime (&ft, pstLoc) ;
     return TRUE ;
}
#endif


static int timeapiPrintDateTime(HANDLE hTZ, LPCTSTR szFormat, LPTSTR szDest, int cbDest, DWORD dwFlags)
{
	MIM_TIMEZONE *tz = (MIM_TIMEZONE*)hTZ;

	if (tz == NULL) return 1;

	SYSTEMTIME st, stl;
	GetSystemTime(&st);

#ifdef _UNICODE 
	if (!SystemTimeToTzSpecificLocalTime(&tz->tzi, &st, &stl))
		return 1;
#else
	if (!MySystemTimeToTzSpecificLocalTime(&tz->tzi, &st, &stl))
		return 1;
#endif
	FormatTime(&stl, szFormat, szDest, cbDest);

	return 0;
}

static int timeapiPrintDateTimeByContact(HANDLE hContact, LPCTSTR szFormat, LPTSTR szDest, int cbDest, DWORD dwFlags)
{
	return timeapiPrintDateTime(timeapiGetInfoByContact(hContact, dwFlags), szFormat, szDest, cbDest, dwFlags);
}

static int timeapiPrepareList(HANDLE hContact, HWND hWnd, DWORD dwFlags)
{
	UINT addMsg, selMsg, findMsg, extMsg;					// control messages for list/combo box

	if (hWnd == NULL)	   // nothing to do
		return 0;

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
	{
		addMsg = CB_ADDSTRING;
		selMsg = CB_SETCURSEL;
		findMsg = CB_FINDSTRING;
		extMsg = CB_SETITEMDATA;
	}
	else if(dwFlags & TZF_PLF_LB) 
	{
		addMsg = LB_ADDSTRING;
		selMsg = LB_SETCURSEL;
		findMsg = LB_FINDSTRING;
		extMsg  = LB_SETITEMDATA;
	}
	else
		return 0;									// shouldn't happen

	SendMessage(hWnd, addMsg, 0, (LPARAM)_T("<unspecified>"));

	if (g_timezonesBias.getCount() == 0) return 0; 

	TCHAR	tszSelectedItem[MIM_TZ_DISPLAYLEN] = _T("");

	/*
	 * preselection by hContact has precedence
	 * if no hContact was given, the tszName will be directly used
	 * for preselecting the item.
	 */

	TCHAR tszName[MIM_TZ_NAMELEN] = _T("");
	if (hContact) 
	{
		DBVARIANT dbv;

		if(!DBGetContactSettingTString(hContact, "UserInfo", "TzName", &dbv)) 
		{
			mir_sntprintf(tszName, MIM_TZ_NAMELEN, _T("%s"), dbv.ptszVal);
			DBFreeVariant(&dbv);
		}
	}

	int iSelection = -1;
	for (int i = 0; i < g_timezonesBias.getCount(); ++i) 
	{
		SendMessage(hWnd, addMsg, 0, (LPARAM)g_timezonesBias[i]->tszDisplay);
		/*
		 * set the adress of our timezone struct as itemdata
		 * caller can obtain it and use it as a pointer to extract all relevant information
		 */
		SendMessage(hWnd, extMsg, (WPARAM)i + 1, (LPARAM)g_timezonesBias[i]);

		// remember the display name to later select it in the listbox
		if (iSelection == -1 && tszName[0] && !_tcsicmp(tszName, g_timezonesBias[i]->tszName))	
		{
			_tcscpy(tszSelectedItem, g_timezonesBias[i]->tszDisplay);
			iSelection = i + 1;
		}
	}
	if (iSelection != -1) 
	{
		SendMessage(hWnd, selMsg, iSelection, 0);
		return iSelection;
	}

	SendMessage(hWnd, selMsg, 0, 0);
	return 0;
}

static void timeapiStoreListResult(HANDLE hContact, HWND hWnd, DWORD dwFlags)
{
	UINT getMsg, extMsg;					// control messages for list/combo box
	
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
	{
		getMsg = CB_GETCURSEL;
		extMsg = CB_GETITEMDATA;
	}
	else if(dwFlags & TZF_PLF_LB) 
	{
		getMsg = LB_GETCURSEL;
		extMsg = LB_GETITEMDATA;
	}
	else
		return;									// shouldn't happen

	LRESULT offset = SendMessage(hWnd, getMsg, 0, 0);
	if (offset > 0) 
	{
		MIM_TIMEZONE *tz = (MIM_TIMEZONE*)SendMessage(hWnd, extMsg, offset, 0);
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

	tmi->createByName                = timeapiGetInfoByName;
	tmi->createByContact             = timeapiGetInfoByContact;
	tmi->storeByContact              = timeapiSetInfoByContact;

	tmi->printCurrentTime            = timeapiPrintDateTime;
	tmi->printCurrentTimeByContact   = timeapiPrintDateTimeByContact;

	tmi->prepareList                = timeapiPrepareList;
	tmi->storeListResults           = timeapiStoreListResult;

	return TRUE;
}


void InitTimeZones(void)
{
	GetTimeZoneInformation(&myInfo.tzi);
	myInfo.timestamp = time(NULL);

	REG_TZI_FORMAT	tzi;
	HKEY			hKey;
	bool			isWin2KPlus = IsWinVer2000Plus();
	DYNAMIC_TIME_ZONE_INFORMATION dtzi = {0};

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
			pfnGetDynamicTimeZoneInformation(&dtzi);
	}


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

				tz->tszDisplay[0] = 0;
				dwLength = sizeof(tz->tszDisplay);
				RegQueryValueEx(hSubKey, _T("Display"), NULL, NULL, (unsigned char *)tz->tszDisplay, &dwLength); 

				szStdName[0] = 0;
				dwLength = sizeof(szStdName);
				RegQueryValueEx(hSubKey, _T("Std"), NULL, NULL, (unsigned char *)szStdName, &dwLength);

				szDayName[0] = 0;
				dwLength = sizeof(szDayName);
				RegQueryValueEx(hSubKey, _T("Dlt"), NULL, NULL, (unsigned char *)szDayName, &dwLength);

#ifndef _UNICODE
				MultiByteToWideChar(CP_ACP, 0, szStdName, -1, tz->tzi.StandardName, SIZEOF(tz->tzi.StandardName));
				MultiByteToWideChar(CP_ACP, 0, szDayName, -1, tz->tzi.DaylightName, SIZEOF(tz->tzi.DaylightName));
#else
				lstrcpyn(tz->tzi.StandardName, szStdName, SIZEOF(tz->tzi.StandardName));
				lstrcpyn(tz->tzi.DaylightName, szDayName, SIZEOF(tz->tzi.DaylightName));
#endif
				if (myInfo.myTZ == NULL)
				{
					if (dtzi.TimeZoneKeyName[0]) 
					{
						if (!myInfo.myTZ && !_tcscmp(tszName, dtzi.TimeZoneKeyName))			// compare registry key names (= not localized, so it's easy)
							myInfo.myTZ = tz;
					}
					else 
					{
						if (isWin2KPlus)
						{
							dwLength = sizeof(szStdName);
							RegQueryValueEx(hSubKey, _T("MUI_Std"), NULL, NULL, (unsigned char *)szStdName, &dwLength); 

							dwLength = sizeof(szDayName);
							RegQueryValueEx(hSubKey, _T("MUI_Dlt"), NULL, NULL, (unsigned char *)szDayName, &dwLength);
						}

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

	CreateServiceFunction(MS_SYSTEM_GET_TMI, GetTimeApi);

	tmi.cbSize = sizeof(tmi);
	GetTimeApi(0, (LPARAM)&tmi);
}

void UninitTimeZones(void)
{
	g_timezonesBias.destroy();
	g_timezones.destroy();
}
