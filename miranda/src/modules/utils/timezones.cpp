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

static INT_PTR svcGetInfoByName(WPARAM wParam, LPARAM lParam)
{
	TCHAR *tszName = (TCHAR*)wParam;
	DWORD  dwFlags = (DWORD)lParam;

	if (tszName == NULL)
		return (dwFlags & (TZF_DIFONLY | TZF_KNOWNONLY)) ? 0 : (INT_PTR)myInfo.myTZ;

#ifdef _UNICODE
	if (!(dwFlags & TZF_UNICODE))
		tszName = mir_a2t((char*)wParam);
#endif

	MIM_TIMEZONE tzsearch;
	tzsearch.hash = hashstr(tszName);

	MIM_TIMEZONE *tz = g_timezones.find(&tzsearch);
	if (tz == NULL)
		return (dwFlags & (TZF_DIFONLY | TZF_KNOWNONLY)) ? 0 : (INT_PTR)myInfo.myTZ;

	if (dwFlags & TZF_DIFONLY)
		return IsSameTime(&tz->tzi) ? 0 : (INT_PTR)tz;

#ifdef _UNICODE
	if (!(dwFlags & TZF_UNICODE))
		mir_free(tszName);
#endif

	return (INT_PTR)tz;
}

static INT_PTR svcGetInfoByContact(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	DWORD  dwFlags = (DWORD)lParam;

	if (hContact == NULL)
		return (dwFlags & (TZF_DIFONLY | TZF_KNOWNONLY)) ? 0 : (INT_PTR)myInfo.myTZ;

	DBVARIANT	dbv;
	if (!DBGetContactSettingTString(hContact, "UserInfo", "TzName", &dbv)) 
	{
		INT_PTR res = svcGetInfoByName((WPARAM)dbv.ptszVal, lParam | TZF_TCHAR);  
		DBFreeVariant(&dbv);
		return res;
	} 
	else 
	{
		/*
		 * try the GMT Bias
		 */
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
					return IsSameTime(&tz->tzi) ? 0 : (INT_PTR)tz;

				return (INT_PTR)tz;
			}
		}
		return (dwFlags & (TZF_DIFONLY | TZF_KNOWNONLY)) ? 0 : (INT_PTR)myInfo.myTZ;
	}
}

static INT_PTR svcSetInfoByContact(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	MIM_TIMEZONE *ptz = (MIM_TIMEZONE*)lParam;

	if (hContact == NULL) return 0;

	if (ptz) 
	{
		DBWriteContactSettingTString(hContact, "UserInfo", "TzName", ptz->tszName);
		DBWriteContactSettingByte(hContact, "UserInfo", "Timezone", (char)((ptz->tzi.Bias + ptz->tzi.StandardBias) / -60));
	} 
	else 
	{
		DBDeleteContactSetting(hContact, "UserInfo", "Timezone");
		DBDeleteContactSetting(hContact, "UserInfo", "TzName");
	}
	return 0;
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


INT_PTR svcPrintDateTime(WPARAM wParam, LPARAM lParam)
{
	TZTOSTRING *tzt  = (TZTOSTRING*)wParam;
	MIM_TIMEZONE *tz;

	if (tzt == NULL) return 1;

	tz = (tzt->flags & TZF_HCONTACT) ? 
		(MIM_TIMEZONE*)svcGetInfoByContact((WPARAM)tzt->hTimeZone, tzt->flags) : 
		(MIM_TIMEZONE*)tzt->hTimeZone;
	
	if (tz == NULL) return 1;

	SYSTEMTIME st, stl;
	GetSystemTime(&st);

#ifdef _UNICODE 
	if (!SystemTimeToTzSpecificLocalTime(&tz->tzi, &st, &stl))
		return 1;

	if (tzt->flags & TZF_UNICODE)
		FormatTime(&stl, tzt->szFormat, tzt->szDest, tzt->cbDest);
	else
		FormatTimeA(&stl, (char*)tzt->szFormat, (char*)tzt->szDest, tzt->cbDest);
#else
	if (!MySystemTimeToTzSpecificLocalTime(&tz->tzi, &st, &stl))
		return 1;

	FormatTime(&stl, tzt->szFormat, tzt->szDest, tzt->cbDest);
#endif

	return 0;
}

static INT_PTR svcPrepareList(WPARAM wParam, LPARAM lParam)
{
	if (lParam == 0)
		return 0;

	MIM_TZ_PREPARELIST *mtzd = (MIM_TZ_PREPARELIST*)lParam;
	UINT	addMsg, selMsg, findMsg, extMsg;					// control messages for list/combo box

	if (mtzd->cbSize != sizeof(MIM_TZ_PREPARELIST))
		return 0;

	if (mtzd->hWnd == NULL)	   // nothing to do
		return 0;

	if (!(mtzd->dwFlags & TZF_PLF_CB || mtzd->dwFlags & TZF_PLF_LB)) 
	{
		/*
		 * figure it out by class name
		 */
		TCHAR	tszClassName[128];
		GetClassName(mtzd->hWnd, tszClassName, SIZEOF(tszClassName));
		if (!_tcsicmp(tszClassName, _T("COMBOBOX")))
			mtzd->dwFlags |= TZF_PLF_CB;
		else if(!_tcsicmp(tszClassName, _T("LISTBOX")))
			mtzd->dwFlags |= TZF_PLF_LB;
	}
	if (mtzd->dwFlags & TZF_PLF_CB) 
	{
		addMsg = CB_ADDSTRING;
		selMsg = CB_SETCURSEL;
		findMsg = CB_FINDSTRING;
		extMsg = CB_SETITEMDATA;
	}
	else if(mtzd->dwFlags & TZF_PLF_LB) 
	{
		addMsg = LB_ADDSTRING;
		selMsg = LB_SETCURSEL;
		findMsg = LB_FINDSTRING;
		extMsg  = LB_SETITEMDATA;
	}
	else
		return 0;									// shouldn't happen

	SendMessage(mtzd->hWnd, addMsg, 0, (LPARAM)_T("<unspecified>"));

	if (g_timezonesBias.getCount() == 0) return 0; 

	TCHAR	tszSelectedItem[MIM_TZ_DISPLAYLEN] = _T("");

	/*
	 * preselection by hContact has precedence
	 * if no hContact was given, the tszName will be directly used
	 * for preselecting the item.
	 */

	if(mtzd->hContact) 
	{
		DBVARIANT dbv;

		if(!DBGetContactSettingTString(mtzd->hContact, "UserInfo", "TzName", &dbv)) 
		{
			mir_sntprintf(mtzd->tszName, MIM_TZ_NAMELEN, _T("%s"), dbv.ptszVal);
			DBFreeVariant(&dbv);
		}
	}

	int iSelection = -1;
	for (int i = 0; i < g_timezonesBias.getCount(); ++i) 
	{
		SendMessage(mtzd->hWnd, addMsg, 0, (LPARAM)g_timezonesBias[i]->tszDisplay);
		/*
		 * set the adress of our timezone struct as itemdata
		 * caller can obtain it and use it as a pointer to extract all relevant information
		 */
		SendMessage(mtzd->hWnd, extMsg, (WPARAM)i + 1, (LPARAM)g_timezonesBias[i]);

		// remember the display name to later select it in the listbox
		if (iSelection == -1 && mtzd->tszName[0] && !_tcsicmp(mtzd->tszName, g_timezonesBias[i]->tszName))	
		{
			mir_sntprintf(tszSelectedItem, MIM_TZ_DISPLAYLEN, _T("%s"), g_timezonesBias[i]->tszDisplay);
			iSelection = i + 1;
		}
	}
	if (iSelection != -1) 
	{
		SendMessage(mtzd->hWnd, selMsg, (WPARAM)iSelection, 0);
		return iSelection;
	}

	return 0;
}

static INT_PTR svcStoreListResult(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)wParam;
	HWND hwnd = (HWND)lParam;

	LRESULT offset = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
	if (offset > 0) 
	{
		MIM_TIMEZONE *ptz = (MIM_TIMEZONE*)SendMessage(hwnd, CB_GETITEMDATA, offset, 0);
		if ((INT_PTR)ptz != CB_ERR && ptz != 0) 
			svcSetInfoByContact((WPARAM)hContact, (LPARAM)ptz);
	} 
	else 
		svcSetInfoByContact((WPARAM)hContact, 0);

	return 0;
}

void InitTimeZones(void)
{
	GetTimeZoneInformation(&myInfo.tzi);
	myInfo.timestamp = time(NULL);

	REG_TZI_FORMAT	tzi;
	HKEY			hKey;

	const TCHAR *tszKey = IsWinVer2000Plus() ?
		_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones") :
		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Time Zones");

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, tszKey, 0, KEY_ENUMERATE_SUB_KEYS, &hKey)) 
	{
		TCHAR	tszName[MIM_TZ_NAMELEN];
		DWORD	dwIndex = 0;
		HKEY	hSubKey;

		TCHAR* myStdName = mir_u2t(myInfo.tzi.StandardName);
		TCHAR* myDayName = mir_u2t(myInfo.tzi.DaylightName);

		DWORD dwSize = SIZEOF(tszName);
		while (ERROR_NO_MORE_ITEMS != RegEnumKeyEx(hKey, dwIndex++, tszName, &dwSize, NULL, NULL, 0, NULL))
		{
			if (ERROR_SUCCESS == RegOpenKeyEx(hKey, tszName, 0, KEY_QUERY_VALUE, &hSubKey)) 
			{
				dwSize = SIZEOF(tszName);

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
				dwLength = SIZEOF(tz->tszDisplay);
				RegQueryValueEx(hSubKey, _T("Display"), NULL, NULL, (unsigned char *)tz->tszDisplay, &dwLength); 

				TCHAR szStdName[MIM_TZ_NAMELEN] = _T("");
				dwLength = SIZEOF(szStdName);
				RegQueryValueEx(hSubKey, _T("Std"), NULL, NULL, (unsigned char *)szStdName, &dwLength);

				TCHAR szDayName[MIM_TZ_NAMELEN] = _T("");
				dwLength = SIZEOF(szDayName);
				RegQueryValueEx(hSubKey, _T("Dlt"), NULL, NULL, (unsigned char *)szDayName, &dwLength); 

				if (!_tcscmp(szStdName, myStdName) || !_tcscmp(szDayName, myDayName))
					myInfo.myTZ = tz;

#ifndef _UNICODE
				MultiByteToWideChar(CP_ACP, 0, szStdName, -1, tz->tzi.StandardName, SIZEOF(tz->tzi.StandardName));
				MultiByteToWideChar(CP_ACP, 0, szDayName, -1, tz->tzi.DaylightName, SIZEOF(tz->tzi.DaylightName));
#else
				lstrcpyn(tz->tzi.StandardName, szStdName, SIZEOF(tz->tzi.StandardName));
				lstrcpyn(tz->tzi.DaylightName, szDayName, SIZEOF(tz->tzi.DaylightName));
#endif

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

	CreateServiceFunction(MS_TZ_GETINFOBYNAME, svcGetInfoByName);
	CreateServiceFunction(MS_TZ_GETINFOBYCONTACT, svcGetInfoByContact);
	CreateServiceFunction(MS_TZ_SETINFOBYCONTACT, svcSetInfoByContact);
	CreateServiceFunction(MS_TZ_PRINTDATETIME, svcPrintDateTime);
	CreateServiceFunction(MS_TZ_PREPARELIST, svcPrepareList);
	CreateServiceFunction(MS_TZ_STORELISTRESULT, svcStoreListResult);
}

void UninitTimeZones(void)
{
	g_timezonesBias.destroy();
	g_timezones.destroy();
}
