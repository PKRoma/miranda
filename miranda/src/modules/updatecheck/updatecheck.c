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
#include "../../core/commonheaders.h"
#ifdef _ALPHA_BASE_

HANDLE hNetlibUser;

#define UC_MOD        "UpdateCheck"
#define UC_ENABLE     "Enable"
#define UC_ENABLE_DEF 1

#define UC_FAILDELAY  (60)      // 1 minute
#define UC_CHECKDELAY (60*60*4) // 4 hours

static void __cdecl _updatethread(void *unused) {
    NETLIBHTTPREQUEST nlhr, *nlreply;
    DWORD dwNow, dwFail, dwCheck;
    
    dwNow = time(NULL);
    dwFail =  DBGetContactSettingDword(NULL, UC_MOD, "LastFail", 0);
    dwCheck =  DBGetContactSettingDword(NULL, UC_MOD, "LastSuccess", 0);
    if (dwNow-dwFail<UC_FAILDELAY || dwNow-dwCheck<UC_CHECKDELAY) return;
    ZeroMemory(&nlhr, sizeof(nlhr));
    nlhr.cbSize = sizeof(nlhr);
    nlhr.requestType = REQUEST_GET;
    nlhr.flags = NLHRF_DUMPASTEXT;
    nlhr.szUrl = "http://miranda.sourceforge.net/testing/checkout.when";
    nlreply = (NETLIBHTTPREQUEST *) CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)hNetlibUser, (LPARAM)&nlhr);
    if (nlreply) {
        if (nlreply->resultCode==200&&nlreply->dataLength) {
            DBVARIANT dbv;
            
            if (!DBGetContactSetting(NULL, UC_MOD, "Result", &dbv)) {
                if (strcmp(dbv.pszVal, nlreply->pData)) {
                    if (IDYES==MessageBox(0, Translate("A new test build is now available.\r\nWould you like to visit the Miranda Alpha Build site to download this update?"), Translate("Miranda Update Available"), MB_YESNO)) {
                        CallService(MS_UTILS_OPENURL, 0, (LPARAM)"http://miranda.sourceforge.net/testing/");
                    }
                    DBWriteContactSettingString(NULL, UC_MOD, "Result", nlreply->pData);
                    DBWriteContactSettingDword(NULL, UC_MOD, "LastSuccess", time(NULL));
                }
				DBFreeVariant(&dbv);
            }
            else DBWriteContactSettingString(NULL, UC_MOD, "Result", nlreply->pData);
        }
        else DBWriteContactSettingDword(NULL, UC_MOD, "LastFail", time(NULL));
		// free HTTP reply
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)nlreply);
    }
    else {
        DBWriteContactSettingDword(NULL, UC_MOD, "LastFail", time(NULL));
    }
}

static int UpdateModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	NETLIBUSER nlu={0};
	nlu.cbSize=sizeof(nlu);
	nlu.flags=NUF_OUTGOING|NUF_HTTPCONNS|NUF_NOHTTPSOPTION;
	nlu.szSettingsModule=UC_MOD;
	nlu.szDescriptiveName=Translate("Miranda Update Check connection");
	hNetlibUser=(HANDLE)CallService(MS_NETLIB_REGISTERUSER,0,(LPARAM)&nlu);
    if (hNetlibUser) {
        forkthread(_updatethread, 0, NULL);
    }
    return 0;
}

static int UpdateModulesUnLoaded(WPARAM wParam, LPARAM lParam)
{
    Netlib_CloseHandle(hNetlibUser);
    return 0;
}

int LoadUpdateCheckModule(void)
{
    if (!DBGetContactSettingByte(NULL, UC_MOD, UC_ENABLE, UC_ENABLE_DEF)) return 0;
    HookEvent(ME_SYSTEM_MODULESLOADED, UpdateModulesLoaded);
    HookEvent(ME_SYSTEM_SHUTDOWN, UpdateModulesUnLoaded);
    return 0;
}

#endif
