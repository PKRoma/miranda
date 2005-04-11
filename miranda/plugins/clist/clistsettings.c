/*

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
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "commonheaders.h"

void InsertContactIntoTree(HANDLE hContact, int status);
extern HWND hwndContactTree;

struct displayNameCacheEntry
{
    HANDLE hContact;
    char *name;
}
static *displayNameCache;
static int displayNameCacheSize;

void InitDisplayNameCache(void)
{
    int i;

    displayNameCacheSize = CallService(MS_DB_CONTACT_GETCOUNT, 0, 0) + 1;
    displayNameCache = (struct displayNameCacheEntry *) mir_realloc(NULL, displayNameCacheSize * sizeof(struct displayNameCacheEntry));
    for (i = 0; i < displayNameCacheSize; i++) {
        displayNameCache[i].hContact = INVALID_HANDLE_VALUE;
        displayNameCache[i].name = NULL;
    }
}

void FreeDisplayNameCache(void)
{
    int i;

    for (i = 0; i < displayNameCacheSize; i++)
        mir_free(displayNameCache[i].name);
    mir_free(displayNameCache);
    displayNameCache = NULL;
    displayNameCacheSize = 0;
}

static int GetDisplayNameCacheEntry(HANDLE hContact)
{
    int i, firstUnused = -1;

    for (i = 0; i < displayNameCacheSize; i++) {
        if (hContact == displayNameCache[i].hContact)
            return i;
        if (firstUnused == -1 && displayNameCache[i].hContact == INVALID_HANDLE_VALUE)
            firstUnused = i;
    }
    if (firstUnused == -1) {
        firstUnused = displayNameCacheSize++;
        displayNameCache =
            (struct displayNameCacheEntry *) mir_realloc(displayNameCache, displayNameCacheSize * sizeof(struct displayNameCacheEntry));
    }
    displayNameCache[firstUnused].hContact = hContact;
    displayNameCache[firstUnused].name = NULL;
    return firstUnused;
}

void InvalidateDisplayNameCacheEntry(HANDLE hContact)
{
    if (hContact == INVALID_HANDLE_VALUE) {
        FreeDisplayNameCache();
        InitDisplayNameCache();
        SendMessage(hwndContactTree, CLM_AUTOREBUILD, 0, 0);
    }
    else {
        int i = GetDisplayNameCacheEntry(hContact);
        mir_free(displayNameCache[i].name);
        displayNameCache[i].name = NULL;
    }
}

int GetContactDisplayName(WPARAM wParam, LPARAM lParam)
{
    CONTACTINFO ci;
    int cacheEntry = -1;
    char *buffer;

    if ((int) lParam != GCDNF_NOMYHANDLE) {
        cacheEntry = GetDisplayNameCacheEntry((HANDLE) wParam);
        if (displayNameCache[cacheEntry].name)
            return (int) displayNameCache[cacheEntry].name;
    }
    ZeroMemory(&ci, sizeof(ci));
    ci.cbSize = sizeof(ci);
    ci.hContact = (HANDLE) wParam;
    if (ci.hContact == NULL)
        ci.szProto = "ICQ";
    ci.dwFlag = (int) lParam == GCDNF_NOMYHANDLE ? CNF_DISPLAYNC : CNF_DISPLAY;
    if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
        if (ci.type == CNFT_ASCIIZ) {
            if (cacheEntry == -1) {
                buffer = (char *) mir_alloc(strlen(ci.pszVal) + 1);
                _snprintf(buffer, strlen(ci.pszVal) + 1, "%s", ci.pszVal);
                mir_free(ci.pszVal);
                return (int) buffer;
            }
            else {
                displayNameCache[cacheEntry].name = ci.pszVal;
                return (int) ci.pszVal;
            }
        }
        if (ci.type == CNFT_DWORD) {
            if (cacheEntry == -1) {
                buffer = (char *) mir_alloc(15);
                _snprintf(buffer, 15, "%u", ci.dVal);
                return (int) buffer;
            }
            else {
                buffer = (char *) mir_alloc(15);
                _snprintf(buffer, 15, "%u", ci.dVal);
                displayNameCache[cacheEntry].name = buffer;
                return (int) buffer;
            }
        }
    }
    CallContactService((HANDLE) wParam, PSS_GETINFO, SGIF_MINIMAL, 0);
    buffer = Translate("(Unknown Contact)");
    //buffer = (char*)mir_alloc(strlen(Translate("'(Unknown Contact)'"))+1);
    //_snprintf(buffer,strlen(Translate("'(Unknown Contact)'"))+1,"%s",Translate("'(Unknown Contact)'"));
    return (int) buffer;
}

int InvalidateDisplayName(WPARAM wParam, LPARAM lParam)
{
    InvalidateDisplayNameCacheEntry((HANDLE) wParam);
    return 0;
}

int ContactAdded(WPARAM wParam, LPARAM lParam)
{
    ChangeContactIcon((HANDLE) wParam, IconFromStatusMode((char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0), ID_STATUS_OFFLINE), 1);
    SortContacts();
    return 0;
}

int ContactDeleted(WPARAM wParam, LPARAM lParam)
{
    CallService(MS_CLUI_CONTACTDELETED, wParam, 0);
    return 0;
}

int ContactSettingChanged(WPARAM wParam, LPARAM lParam)
{
    DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;
    DBVARIANT dbv;

    // Early exit
    if ((HANDLE) wParam == NULL)
        return 0;

    dbv.pszVal = NULL;
    if (!DBGetContactSetting((HANDLE) wParam, "Protocol", "p", &dbv)) {
        if (!strcmp(cws->szModule, dbv.pszVal)) {
            InvalidateDisplayNameCacheEntry((HANDLE) wParam);
            if (!strcmp(cws->szSetting, "UIN") || !strcmp(cws->szSetting, "Nick") || !strcmp(cws->szSetting, "FirstName")
                || !strcmp(cws->szSetting, "LastName") || !strcmp(cws->szSetting, "e-mail")) {
                CallService(MS_CLUI_CONTACTRENAMED, wParam, 0);
            }
            else if (!strcmp(cws->szSetting, "Status")) {
                if (!DBGetContactSettingByte((HANDLE) wParam, "CList", "Hidden", 0)) {
                    if (DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT)) {
                        // User's state is changing, and we are hideOffline-ing
                        if (cws->value.wVal == ID_STATUS_OFFLINE) {
                            ChangeContactIcon((HANDLE) wParam, IconFromStatusMode(cws->szModule, cws->value.wVal), 0);
                            CallService(MS_CLUI_CONTACTDELETED, wParam, 0);
                            mir_free(dbv.pszVal);
                            return 0;
                        }
                        ChangeContactIcon((HANDLE) wParam, IconFromStatusMode(cws->szModule, cws->value.wVal), 1);
                    }
                    ChangeContactIcon((HANDLE) wParam, IconFromStatusMode(cws->szModule, cws->value.wVal), 0);
                }
            }
            else {
                mir_free(dbv.pszVal);
                return 0;
            }
            SortContacts();
        }
    }

    if (!strcmp(cws->szModule, "CList")) {
        if (!strcmp(cws->szSetting, "Hidden")) {
            if (cws->value.type == DBVT_DELETED || cws->value.bVal == 0) {
                char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
                ChangeContactIcon((HANDLE) wParam,
                                  IconFromStatusMode(szProto,
                                                     szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord((HANDLE) wParam, szProto, "Status",
                                                                                                                   ID_STATUS_OFFLINE)), 1);
            }
            else
                CallService(MS_CLUI_CONTACTDELETED, wParam, 0);
        }
        if (!strcmp(cws->szSetting, "MyHandle")) {
            InvalidateDisplayNameCacheEntry((HANDLE) wParam);
        }
    }

    if (!strcmp(cws->szModule, "Protocol")) {
        if (!strcmp(cws->szSetting, "p")) {
            char *szProto;
            if (cws->value.type == DBVT_DELETED)
                szProto = NULL;
            else
                szProto = cws->value.pszVal;
            ChangeContactIcon((HANDLE) wParam,
                              IconFromStatusMode(szProto,
                                                 szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord((HANDLE) wParam, szProto, "Status",
                                                                                                               ID_STATUS_OFFLINE)), 0);
        }
    }

    // Clean up
    if (dbv.pszVal)
        mir_free(dbv.pszVal);

    return 0;

}
