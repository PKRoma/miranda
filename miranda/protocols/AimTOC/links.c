/*
AOL Instant Messenger Plugin for Miranda IM

Copyright � 2003 Robert Rainwater

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

#define AIMWATCHERCLASS "__AIMWatcherClass__"

static ATOM aWatcherClass = 0;
static HWND hWatcher = NULL;

static void aim_links_normalize(char *szMsg)
{
    char *s = szMsg;
    while (*s) {
        if (*s == '+')
            *s = ' ';
        s++;
    }
}

static LRESULT CALLBACK aim_links_watcherwndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_COPYDATA:
        {
            char *szData, *s;
            COPYDATASTRUCT *cds = (COPYDATASTRUCT *) lParam;
            
            LOG(LOG_DEBUG, "Links: WM_COPYDATA");
            // Check to see if link support is enabled
            // We shouldn't need this check since the class instance shouldn't be running
            // but lets be safe
            if (!DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_AL, AIM_KEY_AL_DEF))
                break;
            if (!(char *) cds->lpData)
                break;
            s = szData = _strdup((char *) cds->lpData);
            aim_links_normalize(szData);
            s += 4;
            if (!_strnicmp(s, "goim?", 5)) {
                char *tok, *sn = NULL, *msg = NULL;

                LOG(LOG_DEBUG, "Links: WM_COPYDATA - goim");
                s += 5;
                if (!(*s))
                    break;
                tok = strtok(s, "&");
                while (tok != NULL) {
                    if (!_strnicmp(tok, "screenname=", 11)) {
                        sn = tok + 11;
                    }
                    if (!_strnicmp(tok, "message=", 8)) {
                        msg = tok + 8;
                    }
                    tok = strtok(NULL, "&");
                }
                if (sn && ServiceExists(MS_MSG_SENDMESSAGE)) {
                    HANDLE hContact = aim_buddy_get(sn, 1, 0, 0, NULL);

                    if (hContact)
                        CallService(MS_MSG_SENDMESSAGE, (WPARAM) hContact, (LPARAM) msg);
                }
            }
            if (!_strnicmp(s, "gochat?", 7)) {
                char *tok, *rm = NULL, *ex;
                int exchange = 0;
                
                LOG(LOG_DEBUG, "Links: WM_COPYDATA - gochat");
                s += 7;
                if (!(*s))
                    break;
                tok = strtok(s, "&");
                while (tok != NULL) {
                    if (!_strnicmp(tok, "roomname=", 9)) {
                        rm = tok + 9;
                    }
                    if (!_strnicmp(tok, "exchange=", 9)) {
                        ex = tok + 9;
                        exchange = atoi(ex);
                    }
                    tok = strtok(NULL, "&");
                }
                if (rm && exchange > 0) {
                    aim_gchat_joinrequest(rm, exchange);
                }
            }
            free(szData);
            break;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void aim_links_regwatcher()
{
    WNDCLASS wc;
    
    LOG(LOG_DEBUG, "Links: regwatcher");
    if (hWatcher || aWatcherClass) {
        return;
    }
    memset(&wc, 0, sizeof(WNDCLASS));
    wc.lpfnWndProc = aim_links_watcherwndproc;
    wc.hInstance = hInstance;
    wc.lpszClassName = AIMWATCHERCLASS;
    aWatcherClass = RegisterClass(&wc);
    hWatcher = CreateWindowEx(0, AIMWATCHERCLASS, "", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    return;
}

static void aim_links_unregwatcher()
{
    LOG(LOG_DEBUG, "Links: unregwatcher");
    if (hWatcher) {
        DestroyWindow(hWatcher);
        hWatcher = NULL;
    }
    if (aWatcherClass) {
        UnregisterClass(AIMWATCHERCLASS, hInstance);
        aWatcherClass = 0;
    }
    return;
}

static BOOL CALLBACK aim_linsk_enumthreadwindowsproc(HWND hwnd, LPARAM lParam)
{
    char szBuf[64];

    if (GetClassName(hwnd, szBuf, sizeof(szBuf))) {
        if (!strcmp(szBuf, AIMWATCHERCLASS)) {
            COPYDATASTRUCT cds;
            
            LOG(LOG_DEBUG, "Links: enumthreadwindowsproc - found AIMWATCHERCLASS");
            cds.dwData = 1;
            cds.cbData = strlen((char *) lParam) + 1;
            cds.lpData = (char *) lParam;
            SendMessageTimeout(hwnd, WM_COPYDATA, (WPARAM) hwnd, (LPARAM) & cds, SMTO_ABORTIFHUNG | SMTO_BLOCK, 150, NULL);
        }
    };
    return TRUE;
}

static BOOL CALLBACK aim_linsk_enumwindowsproc(HWND hwnd, LPARAM lParam)
{
    char szBuf[32];
    
    LOG(LOG_DEBUG, "Links: enumwindowsproc");
    if (GetClassName(hwnd, szBuf, sizeof(szBuf))) {
        if (!strcmp(szBuf, MIRANDACLASS)) {
            LOG(LOG_DEBUG, "Links: enumwindowsproc - found Miranda window");
            EnumThreadWindows(GetWindowThreadProcessId(hwnd, NULL), aim_linsk_enumthreadwindowsproc, lParam);
        }
    }
    return TRUE;
}

static void aim_links_register()
{
    HKEY hkey;
    char szBuf[MAX_PATH], szExe[MAX_PATH * 2], szShort[MAX_PATH];
    
    LOG(LOG_DEBUG, "Links: register");
    if (RegCreateKey(HKEY_CLASSES_ROOT, "aim\\shell\\open\\command", &hkey) == ERROR_SUCCESS) {
        GetModuleFileName(hInstance, szBuf, sizeof(szBuf));
        GetShortPathName(szBuf, szShort, sizeof(szShort));
        // MSVC exports differently than gcc/mingw
#ifdef _MSC_VER
        _snprintf(szExe, sizeof(szExe), "RUNDLL32.EXE %s,_aim_links_exec@16 %%1", szShort);
        LOG(LOG_DEBUG, "Links: registering (%s)", szExe);
#else
        _snprintf(szExe, sizeof(szExe), "RUNDLL32.EXE %s,aim_links_exec@16 %%1", szShort);
        LOG(LOG_DEBUG, "Links: registering (%s)", szExe);
#endif
        RegSetValue(hkey, NULL, REG_SZ, szExe, strlen(szExe));
        RegCloseKey(hkey);
    }
    else {
        LOG(LOG_DEBUG, "Links: unregister - unable to create registry key");
    }
}

void aim_links_unregister()
{
    LOG(LOG_DEBUG, "Links: unregister");
    RegDeleteKey(HKEY_CLASSES_ROOT, "aim\\shell\\open\\command");
    RegDeleteKey(HKEY_CLASSES_ROOT, "aim");
}

void aim_links_init()
{
    LOG(LOG_DEBUG, "Links: init");
    if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_AL, AIM_KEY_AL_DEF)) {
        LOG(LOG_DEBUG, "Links: init - links support is on");
        aim_links_register();
        aim_links_regwatcher();
    }
}

void aim_links_destroy()
{
    LOG(LOG_DEBUG, "Links: destroy");
    aim_links_unregwatcher();
}

void __declspec(dllexport)
     CALLBACK aim_links_exec(HWND hwnd, HINSTANCE hInst, char *lpszCmdLine, int nCmdShow)
{
    EnumWindows(aim_linsk_enumwindowsproc, (LPARAM) lpszCmdLine);
}
