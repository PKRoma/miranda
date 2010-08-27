/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2010 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of clist_nicer plugin for Miranda.
 *
 * (C) 2005-2010 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 */

/*
 * read a setting for a contact
 */

#include <commonheaders.h>

TCluiData		cfg::dat = {0};
ClcData*		cfg::clcdat = 0;
TExtraCache* 	cfg::eCache = 0;
int 			cfg::nextCacheEntry = 0, cfg::maxCacheEntry = 0;

CRITICAL_SECTION cfg::cachecs = {0};


void cfg::init()
{
	InitializeCriticalSection(&cachecs);
}

DWORD cfg::getDword(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, DWORD uDefault = 0)
{
	return((DWORD)DBGetContactSettingDword(hContact, szModule, szSetting, uDefault));
}

/*
 * read a setting from our default module (Tab_SRMSG)
 */

DWORD cfg::getDword(const char *szSetting = 0, DWORD uDefault = 0)
{
	return((DWORD)DBGetContactSettingDword(0, DEFAULT_MODULE, szSetting, uDefault));
}

/*
 * read a setting from module only
 */

DWORD cfg::getDword(const char *szModule, const char *szSetting, DWORD uDefault)
{
	return((DWORD)DBGetContactSettingDword(0, szModule, szSetting, uDefault));
}

/*
 * same for bytes now
 */
int cfg::getByte(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, int uDefault = 0)
{
	return(DBGetContactSettingByte(hContact, szModule, szSetting, uDefault));
}

int cfg::getByte(const char *szSetting = 0, int uDefault = 0)
{
	return(DBGetContactSettingByte(0, DEFAULT_MODULE, szSetting, uDefault));
}

int cfg::getByte(const char *szModule, const char *szSetting, int uDefault)
{
	return(DBGetContactSettingByte(0, szModule, szSetting, uDefault));
}

INT_PTR cfg::getTString(const HANDLE hContact, const char *szModule, const char *szSetting, DBVARIANT *dbv)
{
	return(DBGetContactSettingTString(hContact, szModule, szSetting, dbv));
}

INT_PTR cfg::getString(const HANDLE hContact, const char *szModule, const char *szSetting, DBVARIANT *dbv)
{
	return(DBGetContactSettingString(hContact, szModule, szSetting, dbv));
}

/*
 * writer functions
 */

INT_PTR cfg::writeDword(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, DWORD value = 0)
{
	return(DBWriteContactSettingDword(hContact, szModule, szSetting, value));
}

/*
 * write non-contact setting
*/

INT_PTR cfg::writeDword(const char *szModule = 0, const char *szSetting = 0, DWORD value = 0)
{
	return(DBWriteContactSettingDword(0, szModule, szSetting, value));
}

INT_PTR cfg::writeByte(const HANDLE hContact = 0, const char *szModule = 0, const char *szSetting = 0, BYTE value = 0)
{
	return(DBWriteContactSettingByte(hContact, szModule, szSetting, value));
}

INT_PTR cfg::writeByte(const char *szModule = 0, const char *szSetting = 0, BYTE value = 0)
{
	return(DBWriteContactSettingByte(0, szModule, szSetting, value));
}

INT_PTR cfg::writeTString(const HANDLE hContact, const char *szModule = 0, const char *szSetting = 0, const TCHAR *str = 0)
{
	return(DBWriteContactSettingTString(hContact, szModule, szSetting, str));
}

int cfg::getCache(const HANDLE hContact, const char *szProto)
{
    int i, iFound = -1;

    for(i = 0; i < nextCacheEntry; i++) {
        if(eCache[i].hContact == hContact) {
            iFound = i;
            break;
        }
    }
    if(iFound == -1) {
		EnterCriticalSection(&cachecs);
        if(nextCacheEntry == maxCacheEntry) {
            maxCacheEntry += 100;
            cfg::eCache = (TExtraCache *)realloc(cfg::eCache, maxCacheEntry * sizeof(TExtraCache));
        }
        memset(&cfg::eCache[nextCacheEntry], 0, sizeof(TExtraCache));
		cfg::eCache[nextCacheEntry].hContact = hContact;
        memset(cfg::eCache[nextCacheEntry].iExtraImage, 0xff, MAXEXTRACOLUMNS);
        cfg::eCache[nextCacheEntry].iExtraValid = 0;
        cfg::eCache[nextCacheEntry].valid = FALSE;
        cfg::eCache[nextCacheEntry].bStatusMsgValid = 0;
        cfg::eCache[nextCacheEntry].statusMsg = NULL;
        cfg::eCache[nextCacheEntry].status_item = NULL;
        LoadSkinItemToCache(&cfg::eCache[nextCacheEntry], szProto);
        cfg::eCache[nextCacheEntry].dwCFlags = cfg::eCache[nextCacheEntry].timezone = 0;
        cfg::eCache[nextCacheEntry].dwDFlags = DBGetContactSettingDword(hContact, "CList", "CLN_Flags", 0);
        cfg::eCache[nextCacheEntry].dwXMask = CalcXMask(hContact);
		cfg::eCache[nextCacheEntry].timediff = -1;
        GetCachedStatusMsg(nextCacheEntry, const_cast<char *>(szProto));
		cfg::eCache[nextCacheEntry].dwLastMsgTime = INTSORT_GetLastMsgTime(hContact);
        iFound = nextCacheEntry++;
		LeaveCriticalSection(&cachecs);
    }
    return iFound;
}
