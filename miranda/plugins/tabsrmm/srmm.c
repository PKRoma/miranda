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
#include "m_popup.h"

int LoadSendRecvMessageModule(void);
int SplitmsgShutdown(void);

PLUGINLINK *pluginLink;
HINSTANCE g_hInst;

PLUGININFO pluginInfo = {
    sizeof(PLUGININFO),
#ifdef _UNICODE
    #ifdef __GNUWIN32__
        "tabSRMsgW (MINGW32 - unicode)",
    #else    
        "tabSRMsgW (unicode)",
    #endif    
#else
    #ifdef __GNUWIN32__
        "tabSRMsg (MINGW32)",
    #else    
        "tabSRMsg",
    #endif    
#endif
    PLUGIN_MAKE_VERSION(0, 0, 8, 98),
    "Send and receive instant messages, using a split mode interface and tab containers.",
    "SRMM by Miranda Team, tab UI by Nightwish",
    "silvercircle@gmail.com",
    "© 2000-2004 Miranda Project",
    "http://tabsrmm.sourceforge.net",
    0,
    DEFMOD_SRMESSAGE            // replace internal version (if any)
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    g_hInst = hinstDLL;
    return TRUE;
}

__declspec(dllexport)
     PLUGININFO *MirandaPluginInfo(DWORD mirandaVersion)
{
    if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 3, 3, 0))
        return NULL;
    return &pluginInfo;
}

int __declspec(dllexport) Load(PLUGINLINK * link)
{
    pluginLink = link;
    return LoadSendRecvMessageModule();
}

int __declspec(dllexport) Unload(void)
{
    return SplitmsgShutdown();
}

int _DebugPopup(HANDLE hContact, const char *fmt, ...)
{
    POPUPDATA ppd;
    va_list va;
    char    debug[1024];
    int     ibsize = 1023;

    if(!DBGetContactSettingByte(NULL, "Tab_SRMsg", "debuginfo", 0))
        return 0;
    
    if(CallService(MS_POPUP_QUERY, PUQS_GETSTATUS, 0) == 1) {
        ZeroMemory((void *)&ppd, sizeof(ppd));
        ppd.lchContact = hContact;
        ppd.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
        va_start(va, fmt);
        _vsnprintf(debug, ibsize, fmt, va);
        strncpy(ppd.lpzContactName, (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,0), MAX_CONTACTNAME);
        strcpy(ppd.lpzText, "tabSRMM: ");
        strncat(ppd.lpzText, debug, MAX_SECONDLINE - 20);
        ppd.colorText = RGB(0,0,0);
        ppd.colorBack = RGB(255,0,0);
        CallService(MS_POPUP_ADDPOPUP, (WPARAM)&ppd, 0);
    }
    return 0;
}

