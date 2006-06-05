/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003-2005 Robert Rainwater

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
#include "aim.h"

extern PLUGININFO pluginInfo;
PLUGINLINK *pluginLink;
HINSTANCE hInstance;
int aimStatus;
HANDLE hNetlib, hNetlibPeer;
char *szStatus = NULL;
char *szModeMsg = NULL;
char AIM_PROTO[MAX_PATH];

// Event Hooks
static HANDLE hHookOptsInit;
static HANDLE hHookModulesLoaded;
static HANDLE hHookSettingDeleted;
static HANDLE hHookSettingChanged;

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
    hInstance = hInst;
    return TRUE;
}

__declspec(dllexport)
     PLUGININFO *MirandaPluginInfo(DWORD mirandaVersion)
{
    // Only load for 0.4 or greater
    if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 0, 0))
        return NULL;
    return &pluginInfo;
}

int aim_modulesloaded(WPARAM wParam, LPARAM lParam)
{
    NETLIBUSER nlu;
    char szP2P[128];
	DBDeleteContactSetting(NULL, "PluginDisable","AIM.dll");
    mir_snprintf(szP2P, sizeof(szP2P), "%sP2P", AIM_PROTO);
    ZeroMemory(&nlu, sizeof(nlu));
    nlu.cbSize = sizeof(nlu);
    nlu.flags = NUF_OUTGOING | NUF_HTTPCONNS;
    nlu.szSettingsModule = AIM_PROTO;
    nlu.szDescriptiveName = Translate("AOL Instant Messenger server connection");
    hNetlib = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM) & nlu);
    nlu.flags = NUF_OUTGOING;
    nlu.szSettingsModule = szP2P;
    nlu.szDescriptiveName = Translate("AOL Instant Messenger client-to-client connections");
    hNetlibPeer = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM) & nlu);
    hHookOptsInit = HookEvent(ME_OPT_INITIALISE, aim_options_init);
    hHookSettingDeleted = HookEvent(ME_DB_CONTACT_DELETED, aim_util_userdeleted);
    hHookSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, aim_util_dbsettingchanged);
    hServerSideList = importBuddies;
    aim_evil_init();
    aim_userinfo_init();
    if (ServiceExists(MS_GC_REGISTER))
        aim_gchat_init();
    aim_idle_init();
    aim_keepalive_init();
    aim_links_init();
    aim_password_init();
    return 0;
}

int __declspec(dllexport) Load(PLUGINLINK * link)
{
    PROTOCOLDESCRIPTOR pd;

    pluginLink = link;
    srand((unsigned int) time(NULL));   // seed our random number generator
    {
        char *str1;
        char str2[MAX_PATH];
        GetModuleFileName(hInstance, str2, MAX_PATH);
        str1 = strrchr(str2, '\\');
        if (str1 != NULL && (strlen(str1 + 1) > 4)) {
            strncpy(AIM_PROTO, str1 + 1, strlen(str1 + 1) - 4);
            AIM_PROTO[strlen(str1 + 1) - 3] = 0;
        }
        CharUpper(AIM_PROTO);
    }
    log_init();
    LoadLibrary("riched20.dll");
    aim_firstrun_check();
    hHookModulesLoaded = HookEvent(ME_SYSTEM_MODULESLOADED, aim_modulesloaded);
    ZeroMemory(&pd, sizeof(pd));
    pd.cbSize = sizeof(pd);
    pd.szName = AIM_PROTO;
    pd.type = PROTOTYPE_PROTOCOL;
    CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM) & pd);
    pthread_mutex_init(&modeMsgsMutex);
    pthread_mutex_init(&connectionHandleMutex);
    pthread_mutex_init(&buddyMutex);
    aim_filerecv_init();
    aim_services_register(hInstance);
    aim_buddy_offlineall();
    return 0;
}

int __declspec(dllexport) Unload()
{
    if (aim_util_isonline()) {
        aim_toc_disconnect();
    }
    aim_evil_destroy();
    aim_userinfo_destroy();
    if (ServiceExists(MS_GC_REGISTER))
        aim_gchat_destroy();
    aim_idle_destroy();
    aim_keepalive_destroy();
    aim_links_destroy();
    aim_password_destroy();
    aim_buddy_delaydeletefree();
    aim_filerecv_destroy();
    pthread_mutex_destroy(&modeMsgsMutex);
    pthread_mutex_destroy(&connectionHandleMutex);
    pthread_mutex_destroy(&buddyMutex);
    LocalEventUnhook(hHookOptsInit);
    LocalEventUnhook(hHookModulesLoaded);
    LocalEventUnhook(hHookSettingDeleted);
    LocalEventUnhook(hHookSettingChanged);
    LocalEventUnhook(hHookOptsInit);
    Netlib_CloseHandle(hNetlib);
    Netlib_CloseHandle(hNetlibPeer);
    FreeLibrary(GetModuleHandle("riched20"));
    log_destroy();
    return 0;
}
