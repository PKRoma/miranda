/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003 Robert Rainwater

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

static int nIdle = 0;
static HANDLE hIdleHook;

static void aim_idle_set(int idle)
{
    if (aim_util_isonline()) {
        char buf[MSG_LEN];

        mir_snprintf(buf, sizeof(buf), "toc_set_idle %d", idle);
        aim_toc_sflapsend(buf, -1, TYPE_DATA);
        nIdle = idle;
        return;
    }
    nIdle = 0;
}

int aim_idle_hook(WPARAM wParam, LPARAM lParam)
{
    BOOL bIdle = (lParam & IDF_ISIDLE);
    BOOL bPrivacy = (lParam & IDF_PRIVACY);

    if (bPrivacy && nIdle) {
        aim_idle_set(0);
        return 0;
    }
    else if (bPrivacy) {
        return 0;
    }
    else {
        if (bIdle) {
            MIRANDA_IDLE_INFO mii;

            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            CallService(MS_IDLE_GETIDLEINFO, 0, (LPARAM) & mii);
            aim_idle_set(mii.idleTime * 60);
        }
        else
            aim_idle_set(0);
    }
    return 0;
}

void aim_idle_init()
{
    hIdleHook = HookEvent(ME_IDLE_CHANGED, aim_idle_hook);
}

void aim_idle_destroy()
{
    UnhookEvent(hIdleHook);
}
