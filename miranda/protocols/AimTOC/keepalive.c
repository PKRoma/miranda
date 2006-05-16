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

#define KATIMEOUT (1000*90)     // 1000 milliseconds * 60 = 60 secs

static int hTimer = 0;
static void CALLBACK aim_keepalive_timer(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
    if (aim_util_isonline()) {
        LOG(LOG_DEBUG, "Sending keep-alive packet");
        aim_toc_sflapsend("", 0, TYPE_KEEPALIVE);
    }
}

void aim_keepalive_init()
{
    if (!hTimer) {
        hTimer = SetTimer(NULL, 0, KATIMEOUT, aim_keepalive_timer);
    }
}

void aim_keepalive_destroy()
{
    if (hTimer) {
        KillTimer(NULL, hTimer);
        hTimer = 0;
    }
}
