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

implements services to handle location - based timezones, instead of
simple GMT offsets.

$Id: services.cpp 96 2010-08-09 19:18:13Z silvercircle $

*/
#include <commonheaders.h>
#include <m_timezones.h>

typedef struct _REG_TZI_FORMAT
{
    LONG Bias;
    LONG StandardBias;
    LONG DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
} REG_TZI_FORMAT;

/*
 * our internal format
 */

typedef struct _tagTimeZone_int {
	DWORD	cbSize;
	TCHAR	tszName[MIM_TZ_NAMELEN];				// windows name for the time zone
	TCHAR	tszDisplay[MIM_TZ_DISPLAYLEN];			// more descriptive display name (that's what usually appears in dialogs)
	LONG	Bias;						// Standardbias
	LONG	DaylightBias;				// daylight Bias
	SYSTEMTIME StandardDate;			// when DST ends
	SYSTEMTIME DaylightDate;			// when DST begins
	char	GMT_Offset;					// simple GMT offset (+/-, measured in half-hours
	LONG	Offset;						// time offset to local time, in seconds
	SYSTEMTIME CurrentTime;
	time_t	now;
	ULONG	hash;
	DWORD	timestamp;					// last time the offset was calculated. Don't do this on every request, it's a waste.
										// every hour should be sufficient.
} MIM_INT_TIMEZONE;

/*
 * our own time zone information
 */

typedef struct _tagInfo {
	TIME_ZONE_INFORMATION tzi;
	LONG		DaylightInfo;
	SYSTEMTIME 	st;
	FILETIME	ft;
	DWORD		timestamp;					// last time updated
	int			nrZones;					// number of time zones
	MIM_TIMEZONE *myTZ;						// set to my own timezone
} TZ_INT_INFO;


static TZ_INT_INFO			myInfo;
static MIM_INT_TIMEZONE 	*g_timezones;

/*
 * time zone calculations
 */

static char iDays [12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static LONG TZ_TimeCompare(const SYSTEMTIME *target)
{
	if(target->wYear == 0 ) {
		/*
		 * the following covers most cases, only the actual switching months need a more
		 * deeply investigation later.
		 */

		if(myInfo.st.wMonth < target->wMonth )
			return -1;
		if(myInfo.st.wMonth > target->wMonth )
			return 1;

		/*
		 * leap year, fix the february
		 */

		if ((myInfo.st.wYear % 4 == 0) && ((myInfo.st.wYear % 100 != 0) || (myInfo.st.wYear % 400 == 0)))
			iDays[1] = 29 ;

		/*
		 * 1) figure out day of week for the 1st day of the given month
		 * 2) calculate the actual date
		 * 3) convert to file times for comparison
		 */

		SYSTEMTIME	stTemp = {0};
		FILETIME	ft;

		CopyMemory(&stTemp, target, sizeof(SYSTEMTIME));

		stTemp.wDay = 1;
		stTemp.wYear = myInfo.st.wYear;
		SystemTimeToFileTime(&stTemp, &ft);
		FileTimeToSystemTime(&ft, &stTemp);

		stTemp.wDay = (1 + (7 + target->wDayOfWeek - stTemp.wDayOfWeek) % 7);

		if(target->wDay == 5 ) {
			stTemp.wDay = stTemp.wDay + 21;
			if((stTemp.wDay + 7) <= iDays[myInfo.st.wMonth - 1])
				stTemp.wDay += 7;
		}
		else
			stTemp.wDay = (target->wDay - 1) * 7;

		SystemTimeToFileTime(&stTemp, &ft);

		if(CompareFileTime(&myInfo.ft, &ft) < 0)
		   return -1;
		else
			return 1;
	}
	return 0;
}

static LONG TZ_GetTimeZoneOffset(const MIM_INT_TIMEZONE *tzi)
{
	if(tzi->StandardDate.wMonth == 0)
		return 0;

		// standard
	if(tzi->DaylightDate.wMonth < tzi->StandardDate.wMonth ) {
		if(TZ_TimeCompare(&tzi->DaylightDate) < 0 || TZ_TimeCompare(&tzi->StandardDate) > 0 )
			return 0;
		else
			return tzi->DaylightBias;
	}
	else {
		// e.g. brazil or nz (only a few, still important)
		if(TZ_TimeCompare(&tzi->StandardDate) < 0 || TZ_TimeCompare(&tzi->DaylightDate) > 0)
			return tzi->DaylightBias;
		else
			return 0;
	}
	return tzi->DaylightBias;
}

/*
 * calculate the offset (in seconds) for a given time zone structure
 */

static LONG TZ_CalcOffset(MIM_INT_TIMEZONE *tzi)
{
	LONG	targetDL, myDL, timediff;
	time_t  now = time(NULL);

	if((now- myInfo.timestamp) > 1800) {		// refresh our private information
		myInfo.timestamp = now;
		myInfo.DaylightInfo = GetTimeZoneInformation(&myInfo.tzi);
		GetSystemTime(&myInfo.st);
		SystemTimeToFileTime(&myInfo.st, &myInfo.ft);
	}
	timediff = tzi->Bias - myInfo.tzi.Bias;

	if(tzi->DaylightDate.wMonth) {
		/*
		 * DST exists, check whether it applies
		 */
		targetDL = TZ_GetTimeZoneOffset(tzi);
	}
	else
		targetDL = 0;

	myDL = (myInfo.DaylightInfo == TIME_ZONE_ID_DAYLIGHT ? (myInfo.tzi.DaylightDate.wMonth ? myInfo.tzi.DaylightBias : 0) : 0);
	timediff += (targetDL - myDL);
	timediff *= 60;					   	// return it in seconds
	tzi->Offset = timediff;
	tzi->GMT_Offset = timediff / 1800;
	return(timediff);
}

/*
 * refresh the current time for that time zone
 * simply use UNIX time(), add the offset and convert it to a SYSTEMTIME
 * structure.
 */

static void TZ_ForceTimeRefresh(MIM_INT_TIMEZONE *tzi)
{
	time_t	now = time(NULL);
	now -= tzi->Offset;

	tzi->now = now;

	LONGLONG ll;
	FILETIME ft;

	ll = Int32x32To64(now, 10000000) + 116444736000000000;
	ft.dwLowDateTime = (DWORD)ll;
	ft.dwHighDateTime = ll >> 32;

	FileTimeToSystemTime(&ft, &tzi->CurrentTime);
}

/*
 * implementation of services
 */

static INT_PTR svcGetInfoByName(WPARAM wParam, LPARAM lParam)
{
	if(wParam == 0)
		return(reinterpret_cast<UINT_PTR>(myInfo.myTZ));

	TCHAR	*tszName = reinterpret_cast<TCHAR *>(wParam);
	DWORD	hash = hashstr(tszName);
	DWORD	dwFlags = (DWORD)lParam;

	for(int i = 0; i < myInfo.nrZones; i++) {
		if(hash == g_timezones[i].hash) {
			time_t now = time(NULL);
			/*
			 * recalculate the offset if too old, otherwise just return quickly
			*/
			if((now - g_timezones[i].timestamp) > 1800) {
				g_timezones[i].timestamp = now;
				TZ_CalcOffset(&g_timezones[i]);
			}
			if(dwFlags & MIM_PLF_FORCE)
				TZ_ForceTimeRefresh(&g_timezones[i]);

			return(reinterpret_cast<INT_PTR>(&g_timezones[i]));
		}
	}
	return(reinterpret_cast<INT_PTR>(myInfo.myTZ));
}

static INT_PTR svcGetInfoByContact(WPARAM wParam, LPARAM lParam)
{
	if(wParam == 0)
		return 0;

	HANDLE		hContact = (HANDLE)wParam;
	DBVARIANT	dbv;
	int		contact_gmt_diff = 0, timediff = 0;
	DWORD		dwFlags = (DWORD)lParam;

	if (!DBGetContactSettingTString(hContact, "UserInfo", "TzName", &dbv)) 
	{
		TCHAR* tszTzName = dbv.ptszVal;

		DWORD hash = hashstr(tszTzName);
		for(int i = 0; i < myInfo.nrZones; i++) {
			if(hash == g_timezones[i].hash) {
				time_t now = time(NULL);

				if((now - g_timezones[i].timestamp) > 1800) {
					g_timezones[i].timestamp = now;
					TZ_CalcOffset(&g_timezones[i]);
				}
				DBFreeVariant(&dbv);
				if(dwFlags & MIM_PLF_FORCE)
					TZ_ForceTimeRefresh(&g_timezones[i]);

					return (INT_PTR)&g_timezones[i];
			}
		}
		DBFreeVariant(&dbv);
	} 
	else 
	{
		/*
		 * try the GMT offset
		 */
		char* szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		BYTE timezone = DBGetContactSettingByte(hContact, "UserInfo", "Timezone", DBGetContactSettingByte(hContact, szProto, "Timezone", 0xff));

		if(timezone != 0xff) 
		{
			contact_gmt_diff = timezone > 128 ? 256 - timezone : 0 - timezone;
			timediff = -contact_gmt_diff * 60 * 60 / 2;

			for(int i = 0; i < myInfo.nrZones; i++) 
			{
				if((LONG)timediff == g_timezones[i].Offset) 
				{
					time_t now = time(NULL);

					if((now - g_timezones[i].timestamp) > 1800) {
						g_timezones[i].timestamp = now;
						TZ_CalcOffset(&g_timezones[i]);
					}
					if(dwFlags & MIM_PLF_FORCE)
						TZ_ForceTimeRefresh(&g_timezones[i]);

					return (INT_PTR)&g_timezones[i];
				}
			}
		}
	}

	return (INT_PTR)myInfo.myTZ;
}

static INT_PTR svcPrepareList(WPARAM wParam, LPARAM lParam)
{
	if(lParam == NULL)
		return 0;

	MIM_TZ_PREPARELIST *mtzd = reinterpret_cast<MIM_TZ_PREPARELIST *>(lParam);
	UINT	addMsg, selMsg, findMsg, extMsg;					// control messages for list/combo box

	if(mtzd->cbSize != sizeof(MIM_TZ_PREPARELIST))
		return 0;

	if(mtzd->hWnd == 0)	   // nothing to do
		return 0;

	if(!(mtzd->dwFlags & MIM_TZ_PLF_CB || mtzd->dwFlags & MIM_TZ_PLF_LB)) {
		/*
		 * figure it out by class name
		 */
		TCHAR	tszClassName[128];
		GetClassName(mtzd->hWnd, tszClassName, SIZEOF(tszClassName));
		if(!_tcsicmp(tszClassName, _T("COMBOBOX")))
			mtzd->dwFlags |= MIM_TZ_PLF_CB;
		else if(!_tcsicmp(tszClassName, _T("LISTBOX")))
			mtzd->dwFlags |= MIM_TZ_PLF_LB;
	}
	if(mtzd->dwFlags & MIM_TZ_PLF_CB) {
		addMsg = CB_ADDSTRING;
		selMsg = CB_SETCURSEL;
		findMsg = CB_FINDSTRING;
		extMsg = CB_SETITEMDATA;
	}
	else if(mtzd->dwFlags & MIM_TZ_PLF_LB) {
		addMsg = LB_ADDSTRING;
		selMsg = LB_SETCURSEL;
		findMsg = LB_FINDSTRING;
		extMsg  = LB_SETITEMDATA;
	}
	else
		return(0);									// shouldn't happen

	SendMessage(mtzd->hWnd, addMsg, 0, (LPARAM)_T("<unspecified>"));

	if(g_timezones) {
		unsigned i = 0;
		TCHAR	tszSelectedItem[MIM_TZ_DISPLAYLEN] = _T("\0");

		/*
		 * preselection by hContact has precedence
		 * if no hContact was given, the tszName will be directly used
		 * for preselecting the item.
		 */

		if(mtzd->hContact) {
			DBVARIANT dbv;

			if(0 == DBGetContactSettingTString(mtzd->hContact, "UserInfo", "TzName", &dbv)) {
				mir_sntprintf(mtzd->tszName, MIM_TZ_NAMELEN, _T("%s"), dbv.ptszVal);
				DBFreeVariant(&dbv);
			}
		}

		int iSelection = -1;

		while(g_timezones[i].tszDisplay[0]) {
			SendMessage(mtzd->hWnd, addMsg, 0, (LPARAM)g_timezones[i].tszDisplay);
			/*
			 * set the adress of our timezone struct as itemdata
			 * caller can obtain it and use it as a pointer to extract all relevant information
			 */
			SendMessage(mtzd->hWnd, extMsg, (WPARAM)i + 1, (LPARAM)&g_timezones[i]);

			if(mtzd->tszName[0] && !_tcscmp(mtzd->tszName, g_timezones[i].tszName))	{		// remember the display name to later select it in the listbox
				mir_sntprintf(tszSelectedItem, MIM_TZ_DISPLAYLEN, _T("%s"), g_timezones[i].tszDisplay);
				iSelection = i + 1;
			}
			/*
			* if ONLY a GMT offset is known, use it anyway. Works in most cases, but not when DST
			* is different at the target time zone
			*/
			//if(mtzd-> != -1 && (LONG)timediff == reg_timezones[i].Bias)
			//	mir_sntprintf(tszSelectedItemBackup, 256, _T("%s"), reg_timezones[i].tszDisplay);
			i++;
		}
		if(iSelection != -1) {
			SendMessage(mtzd->hWnd, selMsg, (WPARAM)iSelection, 0);
			return(iSelection);
		}
	}
	return 0;
}

void InitTimeZones(void)
{
	myInfo.DaylightInfo = GetTimeZoneInformation(&myInfo.tzi);
	GetSystemTime(&myInfo.st);
	SystemTimeToFileTime(&myInfo.st, &myInfo.ft);
	myInfo.timestamp = time(NULL);

	if(g_timezones == NULL) {
		REG_TZI_FORMAT	tzi;
		TCHAR			tszKey[256];
		HKEY			hKey;

		mir_sntprintf(tszKey, SIZEOF(tszKey), _T("Software\\Microsoft\\Windows%s\\CurrentVersion\\Time Zones"), IsWinVer2000Plus() ? _T(" NT") : _T(""));

		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, tszKey, 0, KEY_READ, &hKey)) {
			TCHAR	tszTzKey[256];
			DWORD	dwIndex = 0;
			DWORD	dwSize = MIM_TZ_NAMELEN, dwLength, dwType, dwNrZones = 0;
			HKEY	hSubKey;
			MIM_INT_TIMEZONE rtTmp;

			RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwNrZones, NULL, NULL, NULL, NULL, NULL, NULL, NULL);		// nr of subkeys
			g_timezones = (MIM_INT_TIMEZONE *)mir_calloc(sizeof(MIM_INT_TIMEZONE) * (dwNrZones + 1));
			myInfo.nrZones = dwNrZones;

			if(g_timezones) {
				while(ERROR_NO_MORE_ITEMS != RegEnumKeyEx(hKey, dwIndex, g_timezones[dwIndex].tszName, &dwSize, NULL, NULL, 0, NULL)) {
					mir_sntprintf(tszTzKey, SIZEOF(tszTzKey), _T("%s\\%s"), tszKey, g_timezones[dwIndex].tszName);
					if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, tszTzKey, 0, KEY_READ, &hSubKey)) {
						dwLength = MIM_TZ_DISPLAYLEN;
						if(ERROR_SUCCESS == RegQueryValueEx(hSubKey, _T("Display"), 0, &dwType, (unsigned char *)g_timezones[dwIndex].tszDisplay, &dwLength)) {
							dwLength = sizeof(tzi);
							if(ERROR_SUCCESS == RegQueryValueEx(hSubKey, _T("TZI"), 0, &dwType, (unsigned char *)&tzi, &dwLength)) {
								g_timezones[dwIndex].Bias = tzi.Bias;			// calculate in seconds
								g_timezones[dwIndex].DaylightBias = tzi.DaylightBias;
								g_timezones[dwIndex].StandardDate = tzi.StandardDate;
								g_timezones[dwIndex].DaylightDate = tzi.DaylightDate;
								g_timezones[dwIndex].hash = hashstr(g_timezones[dwIndex].tszName);
								if(!_tcscmp(g_timezones[dwIndex].tszName, myInfo.tzi.StandardName) || !_tcscmp(g_timezones[dwIndex].tszName, myInfo.tzi.DaylightName))
									myInfo.myTZ = reinterpret_cast<MIM_TIMEZONE *>(&g_timezones[dwIndex]);
							}
						}
						RegCloseKey(hSubKey);
					}
					dwIndex++;
					dwSize = MIM_TZ_NAMELEN;
				}
				/*
				 * sort timezones by BIAS
				 */
				for(unsigned int i = 0; i < dwIndex - 1; i++) {
					for(unsigned int j = 0; j < dwIndex - 1; j++) {
						if(g_timezones[j].Bias < g_timezones[j + 1].Bias) {
							rtTmp = g_timezones[j];
							g_timezones[j] = g_timezones[j + 1];
							g_timezones[j + 1] = rtTmp;
						}
					}
				}
			}
			RegCloseKey(hKey);
		}
	}

	CreateServiceFunction(MS_TZ_GETINFOBYNAME, svcGetInfoByName);
	CreateServiceFunction(MS_TZ_GETINFOBYCONTACT, svcGetInfoByContact);
	CreateServiceFunction(MS_TZ_PREPARELIST, svcPrepareList);
}

void UninitTimeZones(void)
{
	mir_free(g_timezones);
}
