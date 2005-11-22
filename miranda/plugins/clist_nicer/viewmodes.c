/*
MirandaPluginInfo IM: the free IM client for Microsoft* Windows*

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

contact list view modes (CLVM)

$Id$

*/

#include "commonheaders.h"

typedef int (__cdecl *pfnEnumCallback)(char *szName);

/*
 * enumerate all view modes, call the callback function with the mode name
 * useful for filling lists, menus and so on..
 */

int TestCallback(char *szsetting)
{
    _DebugPopup(0, "%s", szsetting);
    return 1;
}

int CLVM_EnumProc(const char *szSetting, LPARAM lParam)
{
    pfnEnumCallback EnumCallback = (pfnEnumCallback)lParam;
    
    if (szSetting == NULL)
        return(1);

    EnumCallback((char *)szSetting);
    return(1);
}

void  CLVM_EnumModes(pfnEnumCallback EnumCallback)
{
    DBCONTACTENUMSETTINGS dbces;

    dbces.pfnEnumProc = CLVM_EnumProc;
    dbces.szModule="Atomic";
    dbces.ofsSettings=0;
    dbces.lParam = (LPARAM)EnumCallback;
    CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);
}

void CLVM_TestEnum()
{
    CLVM_EnumModes(TestCallback);
}
