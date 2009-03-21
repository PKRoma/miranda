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
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include "../srfile/file.h"

static char szMirandaPath[MAX_PATH];
static char szMirandaPathLower[MAX_PATH];

static INT_PTR replaceVars(WPARAM wParam, LPARAM lParam);

static int pathIsAbsolute(const char *path)
{
	if ( strlen(path) <= 2 )
		return 0;
	if ((path[1]==':'&&path[2]=='\\')||(path[0]=='\\'&&path[1]=='\\'))
		return 1;
	return 0;
}

static INT_PTR pathToRelative(WPARAM wParam, LPARAM lParam)
{
	char *pSrc = (char*)wParam;
	char *pOut = (char*)lParam;
	if (!pSrc||!strlen(pSrc)||strlen(pSrc)>MAX_PATH) return 0;
	if (!pathIsAbsolute(pSrc)) {
		mir_snprintf(pOut, MAX_PATH, "%s", pSrc);
		return strlen(pOut);
	}
	else {
		char szTmp[MAX_PATH];

		mir_snprintf(szTmp, SIZEOF(szTmp), "%s", pSrc);
		_strlwr(szTmp);
		if (strstr(szTmp, szMirandaPathLower)) {
			mir_snprintf(pOut, MAX_PATH, "%s", pSrc+strlen(szMirandaPathLower));
			return strlen(pOut);
		}
		else {
			mir_snprintf(pOut, MAX_PATH, "%s", pSrc);
			return strlen(pOut);
		}
	}
}

int pathToAbsolute(const char *pSrc, char *pOut, char* base)
{
	if ( !pSrc || !strlen( pSrc ) || strlen( pSrc ) > MAX_PATH )
		return 0;

	if ( base == NULL )
		base = szMirandaPath;

    char buf[MAX_PATH];
    if ( pSrc[0] < ' ')
		return mir_snprintf( pOut, MAX_PATH, "%s", pSrc );
    else if ( pathIsAbsolute( pSrc ))
        return GetFullPathNameA(pSrc, MAX_PATH, pOut, NULL);
	else if ( pSrc[0] != '\\' )
		mir_snprintf( buf, MAX_PATH, "%s%s", base, pSrc );
	else
		mir_snprintf( buf, MAX_PATH, "%s%s", base, pSrc+1 );

    return GetFullPathNameA(buf, MAX_PATH, pOut, NULL);
}

static INT_PTR pathToAbsolute(WPARAM wParam, LPARAM lParam) 
{
	return pathToAbsolute((char*)wParam, (char*)lParam, szMirandaPath);
}

int CreateDirectoryTree( const char *szDir )
{
	DWORD dwAttributes;
	char *pszLastBackslash, szTestDir[ MAX_PATH ];

	lstrcpynA( szTestDir, szDir, SIZEOF( szTestDir ));
	if (( dwAttributes = GetFileAttributesA( szTestDir )) != 0xffffffff && ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY ))
		return ERROR_ACCESS_DENIED;

	pszLastBackslash = strrchr( szTestDir, '\\' );
	if ( pszLastBackslash == NULL )
		return 0;

	*pszLastBackslash = '\0';
	CreateDirectoryTree( szTestDir );
	*pszLastBackslash = '\\';
	return ( CreateDirectoryA( szTestDir, NULL ) == 0 ) ? GetLastError() : 0;
}

static INT_PTR createDirTree(WPARAM, LPARAM lParam)
{
	if ( lParam == 0 )
		return 1;

	return CreateDirectoryTree(( char* )lParam );
}

#ifdef _UNICODE
static TCHAR szMirandaPathW[MAX_PATH];
static TCHAR szMirandaPathWLower[MAX_PATH];

static int pathIsAbsoluteW(const TCHAR *path)
{
	if ( lstrlen(path) <= 2 )
		return 0;
	if ((path[1]==':'&&path[2]=='\\')||(path[0]=='\\'&&path[1]=='\\'))
		return 1;
	return 0;
}

static INT_PTR pathToRelativeW(WPARAM wParam, LPARAM lParam)
{
	TCHAR *pSrc = (TCHAR*)wParam;
	TCHAR *pOut = (TCHAR*)lParam;
	if ( !pSrc || !lstrlen(pSrc) || lstrlen(pSrc) > MAX_PATH )
		return 0;

	if ( !pathIsAbsoluteW( pSrc ))
		mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc);
	else {
		TCHAR szTmp[MAX_PATH];

		mir_sntprintf(szTmp, SIZEOF(szTmp), _T("%s"), pSrc);
		_tcslwr(szTmp);
		if (_tcsstr(szTmp, szMirandaPathWLower))
			mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc+lstrlen(szMirandaPathWLower));
		else
			mir_sntprintf(pOut, MAX_PATH, _T("%s"), pSrc);
	}
	return lstrlen(pOut);
}

int pathToAbsoluteW(const TCHAR *pSrc, TCHAR *pOut, TCHAR* base)
{
	if ( !pSrc || !wcslen(pSrc) || wcslen(pSrc) > MAX_PATH)
		return 0;

	if ( base == NULL )
		base = szMirandaPathW;

    TCHAR buf[MAX_PATH];
    if ( pSrc[0] < ' ')
		return mir_sntprintf( pOut, MAX_PATH, _T("%s"), pSrc );
    else if ( pathIsAbsoluteW( pSrc ))
        return GetFullPathName(pSrc, MAX_PATH, pOut, NULL);
	else if ( pSrc[0] != '\\' )
		mir_sntprintf( buf, MAX_PATH, _T("%s%s"), base, pSrc );
	else
		mir_sntprintf( buf, MAX_PATH, _T("%s%s"), base, pSrc+1 );

    return GetFullPathName(buf, MAX_PATH, pOut, NULL);
}

static INT_PTR pathToAbsoluteW(WPARAM wParam, LPARAM lParam)
{
	return pathToAbsoluteW((TCHAR*)wParam, (TCHAR*)lParam, szMirandaPathW);
}

int CreateDirectoryTreeW( const WCHAR* szDir )
{
	DWORD  dwAttributes;
	WCHAR* pszLastBackslash, szTestDir[ MAX_PATH ];

	lstrcpynW( szTestDir, szDir, SIZEOF( szTestDir ));
	if (( dwAttributes = GetFileAttributesW( szTestDir )) != 0xffffffff && ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY ))
		return ERROR_ACCESS_DENIED;

	pszLastBackslash = wcsrchr( szTestDir, '\\' );
	if ( pszLastBackslash == NULL )
		return 0;

	*pszLastBackslash = '\0';
	CreateDirectoryTreeW( szTestDir );
	*pszLastBackslash = '\\';
	return ( CreateDirectoryW( szTestDir, NULL ) == 0 ) ? GetLastError() : 0;
}

static INT_PTR createDirTreeW(WPARAM, LPARAM lParam)
{
	if ( lParam == 0 )
		return 1;

	CreateDirectoryTreeW(( WCHAR* )lParam );
	return 0;
}

int InitPathUtilsW(void)
{
	TCHAR *p = 0;
	GetModuleFileName(hMirandaInst, szMirandaPathW, SIZEOF(szMirandaPathW));
	p = _tcsrchr(szMirandaPathW,'\\');
	if ( p )
		p[1] = 0;
	mir_sntprintf(szMirandaPathWLower, SIZEOF(szMirandaPathWLower), _T("%s"), szMirandaPathW);
	_tcslwr(szMirandaPathWLower);
	CreateServiceFunction(MS_UTILS_PATHTORELATIVEW, pathToRelativeW);
	CreateServiceFunction(MS_UTILS_PATHTOABSOLUTEW, pathToAbsoluteW);
	CreateServiceFunction(MS_UTILS_CREATEDIRTREEW, createDirTreeW);
	return 0;
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// Variables parser

#define XSTR(target, s) _xstrselect(target, s, _T(s))

static __forceinline int _xcscmp(const char *s1, const char *s2) { return strcmp(s1, s2); }
static __forceinline int _xcsncmp(const char *s1, const char *s2, int n) { return strncmp(s1, s2, n); }
static __forceinline size_t _xcslen(const char *s1) { return strlen(s1); }
static __forceinline char *_xcscpy(char *s1, const char *s2) { return strcpy(s1, s2); }
static __forceinline char *_xcsncpy(char *s1, const char *s2, int n) { return strncpy(s1, s2, n); }
static __forceinline char *_xstrselect(char *, char *s1, TCHAR *s2) { return s1; }
static __forceinline char *_itox(char *, int a) { return itoa(a, (char *)mir_alloc(sizeof(char)*20), 10); }
static __forceinline char *mir_a2x(char *, char *s) { return mir_strdup(s); }
static __forceinline char *GetContactNickX(char *, HANDLE hContact)
{
	return mir_strdup((char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0));
}
static __forceinline char *GetEnvironmentVariableX(char *variable)
{
	char result[512];
	if (GetEnvironmentVariableA(variable, result, SIZEOF(result)))
		return mir_strdup(result);
	return NULL;
}
static __forceinline char *SHGetSpecialFolderPathX(int iCSIDL, char* var)
{
	char result[512];
	if (shGetSpecialFolderPathA && shGetSpecialFolderPathA(NULL, result, iCSIDL, FALSE))
		return mir_strdup(result);
	return NULL;
}
static __forceinline char *GetModulePathX(char *, HMODULE hModule)
{
	char result[MAX_PATH];
	GetModuleFileNameA(hModule, result, sizeof(result));
	PathRemoveFileSpecA(result);
	return mir_strdup(result);
}
static __forceinline char *GetUserNameX(char *)
{
	char result[128];
    DWORD size = SIZEOF(result);
	if (GetUserNameA(result, &size))
		return mir_strdup(result);
	return NULL;
}

#ifdef _UNICODE
static __forceinline int _xcscmp(const TCHAR *s1, const TCHAR *s2) { return _tcscmp(s1, s2); }
static __forceinline int _xcsncmp(const TCHAR *s1, const TCHAR *s2, int n) { return _tcsncmp(s1, s2, n); }
static __forceinline size_t _xcslen(const TCHAR *s1) { return _tcslen(s1); }
static __forceinline TCHAR *_xcscpy(TCHAR *s1, const TCHAR *s2) { return _tcscpy(s1, s2); }
static __forceinline TCHAR *_xcsncpy(TCHAR *s1, const TCHAR *s2, int n) { return _tcsncpy(s1, s2, n); }
static __forceinline TCHAR *_xstrselect(TCHAR *, char *s1, TCHAR *s2) { return s2; }
static __forceinline TCHAR *_itox(TCHAR *, int a) { return _itot(a, (TCHAR *)mir_alloc(sizeof(TCHAR)*20), 10); }
static __forceinline TCHAR *mir_a2x(TCHAR *, char *s) { return mir_a2t(s); }
static __forceinline TCHAR *GetContactNickX(TCHAR *, HANDLE hContact)
{
	return mir_tstrdup((TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR));
}
static __forceinline TCHAR *GetEnvironmentVariableX(TCHAR *variable)
{
	TCHAR result[512];
	if (GetEnvironmentVariable(variable, result, SIZEOF(result)))
		return mir_tstrdup(result);
	return NULL;
}
static __forceinline TCHAR *SHGetSpecialFolderPathX(int iCSIDL, TCHAR* var)
{
	TCHAR result[512];
	if (shGetSpecialFolderPath && shGetSpecialFolderPath(NULL, result, iCSIDL, FALSE))
		return mir_tstrdup(result);
	return NULL;
}
static __forceinline TCHAR *GetModulePathX(TCHAR *, HMODULE hModule)
{
	TCHAR result[MAX_PATH];
	GetModuleFileName(hModule, result, sizeof(result));
	PathRemoveFileSpec(result);
	return mir_tstrdup(result);
}
static __forceinline TCHAR *GetUserNameX(TCHAR *)
{
	TCHAR result[128];
    DWORD size = SIZEOF(result);
	if (GetUserName(result, &size))
		return mir_tstrdup(result);
	return NULL;
}
#endif

template<typename XCHAR>
XCHAR *GetInternalVariable(XCHAR *key, int keyLength, HANDLE hContact)
{
	XCHAR *theValue = NULL;
	XCHAR *theKey = (XCHAR *)_alloca(sizeof(XCHAR) * (keyLength + 1));
	_xcsncpy(theKey, key, keyLength);
	theKey[keyLength] = 0;

	if (hContact) {
		if (!_xcscmp(theKey, XSTR(key, "nick")))
			theValue = GetContactNickX(key, hContact);
		else if (!_xcscmp(theKey, XSTR(key, "proto")))
			theValue = mir_a2x(key, (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact,0));
		else if (!_xcscmp(theKey, XSTR(key, "userid"))) {
			CONTACTINFO ci = {0};
			ci.cbSize = sizeof(ci);
			ci.hContact = hContact;
			ci.szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
			ci.dwFlag = CNF_UNIQUEID;
			if (sizeof(XCHAR) == sizeof(WCHAR)) ci.dwFlag |= CNF_UNICODE;
			if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
				switch (ci.type) {
				case CNFT_ASCIIZ:
					theValue = (XCHAR *)ci.pszVal;
					break;
				case CNFT_DWORD:
					theValue = _itox(key, ci.dVal);
					break;
		}	}	}
	}
	else {
		if (!theValue) {
			if (!_xcscmp(theKey, XSTR(key, "miranda_path")))
				theValue = GetModulePathX(key, NULL);
			else if (!_xcscmp(theKey, XSTR(key, "appdata")))
				theValue = SHGetSpecialFolderPathX(CSIDL_APPDATA, theKey);
			else if (!_xcscmp(theKey, XSTR(key, "mydocuments")))
				theValue = SHGetSpecialFolderPathX(CSIDL_PERSONAL, theKey);
			else if (!_xcscmp(theKey, XSTR(key, "desktop")))
				theValue = SHGetSpecialFolderPathX(CSIDL_DESKTOPDIRECTORY, theKey);
			else if (!_xcscmp(theKey, XSTR(key, "miranda_profile"))) {
				char szProfilePath[MAX_PATH];
				CallService(MS_DB_GETPROFILEPATH, SIZEOF(szProfilePath), (LPARAM) szProfilePath);
				theValue = mir_a2x(key, szProfilePath);
			}
			else if (!_xcscmp(theKey, XSTR(key, "miranda_profilename"))) {
				char szProfileName[MAX_PATH];
				CallService(MS_DB_GETPROFILENAME, SIZEOF(szProfileName), (LPARAM) szProfileName);
				char *pos = strrchr(szProfileName, '.');
				if ( lstrcmpA( pos, ".dat" ) == 0 )
					*pos = 0;
				theValue = mir_a2x(key, szProfileName);
 		    }	
			else if (!_xcscmp(theKey, XSTR(key, "username")))
				theValue = GetUserNameX(key);
            else if (!_xcscmp(theKey, XSTR(key, "miranda_userdata")) 
                        || !_xcscmp(theKey, XSTR(key, "miranda_avatarcache"))
                        || !_xcscmp(theKey, XSTR(key, "miranda_logpath"))) {
                char szFullPath[MAX_PATH], szProfilePath[MAX_PATH], szProfileName[MAX_PATH];
				CallService(MS_DB_GETPROFILEPATH, SIZEOF(szProfilePath), (LPARAM) szProfilePath);
				CallService(MS_DB_GETPROFILENAME, SIZEOF(szProfileName), (LPARAM) szProfileName);
				char *pos = strrchr(szProfileName, '.');
				if ( lstrcmpA( pos, ".dat" ) == 0 )
					*pos = 0;
                if (!_xcscmp(theKey, XSTR(key, "miranda_avatarcache"))) 
                    mir_snprintf(szFullPath, SIZEOF(szFullPath), "%s\\Profiles\\%s\\AvatarCache", szProfilePath, szProfileName);
                else if (!_xcscmp(theKey, XSTR(key, "miranda_logpath"))) 
                    mir_snprintf(szFullPath, SIZEOF(szFullPath), "%s\\Profiles\\%s\\Logs", szProfilePath, szProfileName);
                else mir_snprintf(szFullPath, SIZEOF(szFullPath), "%s\\Profiles\\%s", szProfilePath, szProfileName);
                theValue = mir_a2x(key, szFullPath);
            }
        }

		if (!theValue)
			theValue = GetEnvironmentVariableX(theKey);
	}

	return theValue;
}

template<typename XCHAR>
XCHAR *GetVariableFromArray(REPLACEVARSARRAY *vars, XCHAR *key, size_t keyLength, HANDLE hContact, bool *bFree)
{
	*bFree = false;
	for (REPLACEVARSARRAY *var = vars; var && var->lptzKey; ++var)
		if ((_xcslen((XCHAR *)var->lptzKey) == keyLength) && !_xcsncmp(key, (XCHAR *)var->lptzKey, keyLength))
			return (XCHAR *)var->lptzValue;

	*bFree = true;
	return GetInternalVariable(key, keyLength, hContact);
}

template<typename XCHAR>
XCHAR *ReplaceVariables(XCHAR *str, REPLACEVARSDATA *data)
{
	if (!str)
		return NULL;

	XCHAR *p;
	XCHAR *varStart = 0;
	size_t length = 0;
	bool bFree;

	for (p = str; *p; ++p) {
		if (*p == '%') {
			if (varStart) {
				if (p == varStart)
					length++;
				else if (XCHAR *value = GetVariableFromArray(data->variables, varStart, p-varStart, data->hContact, &bFree)) {
					length += _xcslen(value);
					if (bFree) mir_free(value);
				}
				else // variable not found
					length += p-varStart+2;

				varStart = 0;
			}
			else varStart = p+1;
		}
		else if (!varStart)
			length++;
	}

	XCHAR *result = (XCHAR *)mir_alloc(sizeof(XCHAR) * (length + 1));
	XCHAR *q = result;
	varStart = NULL;

	for (p = str; *p; ++p) {
		if (*p == '%') {
			if (varStart) {
				if (p == varStart)
					*q++ = '%';
				else if (XCHAR *value = GetVariableFromArray(data->variables, varStart, p-varStart, data->hContact, &bFree)) {
					_xcscpy(q, value);
					q += _xcslen(value);
					if (bFree) mir_free(value);
				}
				else {
					// variable not found
					_xcsncpy(q, varStart-1, p-varStart+2);
					q += p-varStart+2;
				}
				varStart = 0;
			}
			else varStart = p+1;
		}
		else if (!varStart)
			*q++ = *p;
	}

	*q = 0;

	return result;
}

static INT_PTR replaceVars(WPARAM wParam, LPARAM lParam)
{
	REPLACEVARSDATA *data = (REPLACEVARSDATA *)lParam;
	if (!(data->dwFlags & RVF_UNICODE))
		return (INT_PTR)ReplaceVariables<char>((char *)wParam, data);

#ifdef _UNICODE
	return (int)ReplaceVariables<WCHAR>((WCHAR *)wParam, data);
#else
	return NULL;
#endif
}

int InitPathUtils(void)
{
	char *p = 0;
	GetModuleFileNameA(hMirandaInst, szMirandaPath, SIZEOF(szMirandaPath));
	p = strrchr(szMirandaPath,'\\');
	if ( p )
		p[1] = 0;
	mir_snprintf(szMirandaPathLower, MAX_PATH, "%s", szMirandaPath);
	_strlwr(szMirandaPathLower);
	CreateServiceFunction(MS_UTILS_PATHTORELATIVE, pathToRelative);
	CreateServiceFunction(MS_UTILS_PATHTOABSOLUTE, pathToAbsolute);
	CreateServiceFunction(MS_UTILS_CREATEDIRTREE, createDirTree);
	CreateServiceFunction(MS_UTILS_REPLACEVARS, replaceVars);
#ifdef _UNICODE
	return InitPathUtilsW();
#else
	return 0;
#endif
}
