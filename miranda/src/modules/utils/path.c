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

static char szMirandaPath[MAX_PATH];

static int pathIsAbsolute(char *path)
{
    if (!path||!strlen(path)>2) return 0;
    if ((path[1]==':'&&path[2]=='\\')||(path[0]=='\\'&&path[1]=='\\')) return 1;
    return 0;
}

static int pathToRelative(WPARAM wParam, LPARAM lParam)
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
        if (strstr(szTmp, szMirandaPath)) {
            mir_snprintf(pOut, MAX_PATH, "%s", pSrc+strlen(szMirandaPath));
            return strlen(pOut);
        }
        else {
            mir_snprintf(pOut, MAX_PATH, "%s", pSrc);
            return strlen(pOut);
        }
    }
}

static int pathToAbsolute(WPARAM wParam, LPARAM lParam) {
    char *pSrc = (char*)wParam;
    char *pOut = (char*)lParam;
    if (!pSrc||!strlen(pSrc)||strlen(pSrc)>MAX_PATH) return 0;
    if (pathIsAbsolute(pSrc)||!isalnum(pSrc[0])) {
        mir_snprintf(pOut, MAX_PATH, "%s", pSrc);
        return strlen(pOut);
    }
    else {
        mir_snprintf(pOut, MAX_PATH, "%s%s", szMirandaPath, pSrc);
        return strlen(pOut);
    }
}

int InitPathUtils(void)
{
	char *p = 0;
	GetModuleFileNameA(GetModuleHandle(NULL), szMirandaPath, SIZEOF(szMirandaPath));
	p=strrchr(szMirandaPath,'\\');
	if (p&&p+1) *(p+1)=0;
    _strlwr(szMirandaPath);
    CreateServiceFunction(MS_UTILS_PATHTORELATIVE, pathToRelative);
    CreateServiceFunction(MS_UTILS_PATHTOABSOLUTE, pathToAbsolute);
    return 0;
}
