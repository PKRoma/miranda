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
	#if defined( _UNICODE )
		wchar_t *wszName;
	#endif
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
		#if defined( _UNICODE )
			displayNameCache[i].wszName = NULL;
		#endif	
	}
}

void FreeDisplayNameCache(void)
{
	int i;

	for (i = 0; i < displayNameCacheSize; i++) {
		mir_free(displayNameCache[i].name);
		#if defined( _UNICODE )
			mir_free(displayNameCache[i].wszName);
		#endif	
	}

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
	#if defined( _UNICODE )
		displayNameCache[firstUnused].wszName = NULL;
	#endif	
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
		#if defined( _UNICODE )
			mir_free(displayNameCache[i].wszName);
			displayNameCache[i].wszName = NULL;
		#endif	
	}
}

TCHAR* GetContactDisplayNameW( HANDLE hContact, int mode )
{
	CONTACTINFO ci;
	int cacheEntry = -1;
	TCHAR *buffer;

	if ( mode != GCDNF_NOMYHANDLE) {
		cacheEntry = GetDisplayNameCacheEntry( hContact );
		#if defined( _UNICODE )
			if (displayNameCache[cacheEntry].wszName)
				return displayNameCache[cacheEntry].wszName;
		#else
			if (displayNameCache[cacheEntry].name)
				return displayNameCache[cacheEntry].name;
		#endif
	}
	ZeroMemory(&ci, sizeof(ci));
	ci.cbSize = sizeof(ci);
	ci.hContact = hContact;
	if (ci.hContact == NULL)
		ci.szProto = "ICQ";
	ci.dwFlag = (mode == GCDNF_NOMYHANDLE) ? CNF_DISPLAYNC : CNF_DISPLAY;
	#if defined( _UNICODE )
		ci.dwFlag += CNF_UNICODE;
	#endif
	if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
		if (ci.type == CNFT_ASCIIZ) {
			if (cacheEntry == -1) {
				size_t len = _tcslen(ci.pszVal);
				buffer = (TCHAR*) mir_alloc( sizeof( TCHAR )*( len+1 ));
				memcpy( buffer, ci.pszVal, len * sizeof( TCHAR ));
				buffer[ len ] = 0;
				mir_free(ci.pszVal);
				return buffer;
			}
			else {
				#if defined( _UNICODE )
					displayNameCache[cacheEntry].wszName = ci.pszVal;
					displayNameCache[cacheEntry].name = u2a( ci.pszVal );
				#else
					displayNameCache[cacheEntry].name = ci.pszVal;
				#endif
				return ci.pszVal;
		}	}

		if (ci.type == CNFT_DWORD) {
			if (cacheEntry == -1) {
				buffer = (TCHAR*) mir_alloc(15 * sizeof( TCHAR ));
				_ltot(ci.dVal, buffer, 10 );
				return buffer;
			}
			else {
				buffer = (TCHAR*) mir_alloc(15 * sizeof( TCHAR ));
				_ltot(ci.dVal, buffer, 10 );
				#if defined( _UNICODE )
					displayNameCache[cacheEntry].wszName = buffer;
					displayNameCache[cacheEntry].name = u2a( buffer );
				#else
					displayNameCache[cacheEntry].name = buffer;
				#endif
				return buffer;
	}	}	}

	CallContactService(hContact, PSS_GETINFO, SGIF_MINIMAL, 0);
	buffer = TranslateT("(Unknown Contact)");
	return buffer;
}

int GetContactDisplayName(WPARAM wParam, LPARAM lParam)
{
	CONTACTINFO ci;
	int cacheEntry = -1;
	char *buffer;

	if ( lParam & GCDNF_UNICODE )
		return ( int )GetContactDisplayNameW(( HANDLE )wParam, lParam & ~GCDNF_UNICODE );

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
	#if defined( _UNICODE )
		ci.dwFlag += CNF_UNICODE;
	#endif
	if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
		if (ci.type == CNFT_ASCIIZ) {
			if (cacheEntry == -1) {
				size_t len = _tcslen(ci.pszVal);
				#if defined( _UNICODE )
					buffer = u2a( ci.pszVal );
					mir_free(ci.pszVal);
				#else
					buffer = ci.pszVal;
				#endif
				return (int) buffer;
			}
			else {
				#if defined( _UNICODE )
					displayNameCache[cacheEntry].wszName = ci.pszVal;
					displayNameCache[cacheEntry].name = u2a( ci.pszVal );
				#else
					displayNameCache[cacheEntry].name = ci.pszVal;
				#endif
				return (int) displayNameCache[cacheEntry].name;
			}
		}
		if (ci.type == CNFT_DWORD) {
			if (cacheEntry == -1) {
				buffer = ( char* )mir_alloc(15);
				ltoa(ci.dVal, buffer, 10 );
				return (int) buffer;
			}
			else {
				buffer = ( char* )mir_alloc(15);
				ltoa(ci.dVal, buffer, 10 );
				displayNameCache[cacheEntry].name = buffer;
				#if defined( _UNICODE )
					displayNameCache[cacheEntry].wszName = a2u( buffer );
				#endif
				return (int) buffer;
	}	}	}

	CallContactService((HANDLE) wParam, PSS_GETINFO, SGIF_MINIMAL, 0);
	buffer = Translate("(Unknown Contact)");
	//buffer = (char*)mir_alloc(strlen(Translate("'(Unknown Contact)'"))+1);
	//mir_snprintf(buffer,strlen(Translate("'(Unknown Contact)'"))+1,"%s",Translate("'(Unknown Contact)'"));
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

/*-----------------------------------------------------*/

char* u2a( wchar_t* src )
{
	int cbLen = WideCharToMultiByte( CP_ACP, 0, src, -1, NULL, 0, NULL, NULL );
	char* result = ( char* )mir_alloc( cbLen+1 );
	if ( result == NULL )
		return NULL;

	WideCharToMultiByte( CP_ACP, 0, src, -1, result, cbLen, NULL, NULL );
	result[ cbLen ] = 0;
	return result;
}

wchar_t* a2u( char* src )
{
	int cbLen = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
	wchar_t* result = ( wchar_t* )mir_alloc( sizeof( wchar_t )*(cbLen+1));
	if ( result == NULL )
		return NULL;

	MultiByteToWideChar( CP_ACP, 0, src, -1, result, cbLen );
	result[ cbLen ] = 0;
	return result;
}

