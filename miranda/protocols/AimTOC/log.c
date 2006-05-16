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

#if defined(DEBUGMODE) || defined(AIM_CVSBUILD)
#define log_level LOG_DEBUG
#else
#define log_level LOG_WARN
#endif

#ifdef AIM_CVSBUILD
static FILE *lfp = 0;
#endif

int LOG(int level, const char *fmt, ...)
{
    va_list va;
    char szText[1024];

    if (log_level > level)
        return 0;
    if (!hNetlib)
        return 0;
    va_start(va, fmt);
    mir_vsnprintf(szText, sizeof(szText), fmt, va);
    va_end(va);
#ifdef AIM_CVSBUILD
    if (lfp) {
        fwrite("[Core] ", strlen("[Core] "), 1, lfp);
        fwrite(szText, strlen(szText), 1, lfp);
        fwrite("\r\n", strlen("\n"), 1, lfp);
    }
#endif
    return CallService(MS_NETLIB_LOG, (WPARAM) hNetlib, (LPARAM) szText);
}

int PLOG(int level, const char *fmt, ...)
{
    va_list va;
    char szText[1024];

    if (log_level > level)
        return 0;
    if (!hNetlibPeer)
        return 0;
    va_start(va, fmt);
    mir_vsnprintf(szText, sizeof(szText), fmt, va);
    va_end(va);
#ifdef AIM_CVSBUILD
    if (lfp) {
        fwrite("[Peer] ", strlen("[Peer] "), 1, lfp);
        fwrite(szText, strlen(szText), 1, lfp);
        fwrite("\r\n", strlen("\r\n"), 1, lfp);
    }
#endif
    return CallService(MS_NETLIB_LOG, (WPARAM) hNetlibPeer, (LPARAM) szText);
}

#ifdef AIM_CVSBUILD
static void GetMirandaPath(char *szPath, int cbPath)
{
    char *str;

    GetModuleFileName(GetModuleHandle(NULL), szPath, cbPath);
    str = strrchr(szPath, '\\');
    if (str != NULL)
        *str = 0;
}
#endif

void log_init()
{
#ifdef AIM_CVSBUILD
    char szPath[MAX_PATH], szFullPath[MAX_PATH];

    if (DBGetContactSettingByte(NULL, AIM_PROTO, "LogToFile", 0)) {
        GetMirandaPath(szPath, sizeof(szPath));
        mir_snprintf(szFullPath, sizeof(szFullPath), "%s\\aim.log");
        lfp = fopen(szFullPath, "w");
    }
#endif
}

void log_destroy()
{
#ifdef AIM_CVSBUILD
    if (lfp)
        fclose(lfp);
#endif
}
