////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include "gg.h"

/* NOTE: Eventhough SetTimer seems to support UINT_PTR for idEvent, it seems that TimerProc
 * does not get full pointer but just 2 byte lower bytes.
 */
#define MAX_TIMERS 8
GGPROTO *g_timers[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static void CALLBACK gg_keepalive(HWND hwnd, UINT message, UINT_PTR idEvent, DWORD dwTime)
{
	GGPROTO *gg = g_timers[idEvent];
	if (gg_isonline(gg))
	{
#ifdef DEBUGMODE
		gg_netlog("Sending keep-alive");
#endif
		gg_ping(gg->sess);
	}
}

void gg_keepalive_init(GGPROTO *gg)
{
	if (DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_KEEPALIVE, GG_KEYDEF_KEEPALIVE))
	{
		int i;
		for(i = 0; i < MAX_TIMERS && g_timers[i] == NULL; i++);
		if(i < MAX_TIMERS)
		{
			gg->timer = SetTimer(NULL, i, 1000 * 60, gg_keepalive);
			g_timers[i] = gg;
		}
	}
}

void gg_keepalive_destroy(GGPROTO *gg)
{
#ifdef DEBUGMODE
	gg_netlog("gg_destroykeepalive(): Killing Timer");
#endif
	if (gg->timer)
	{
		int i;
		KillTimer(NULL, gg->timer);
		for(i = 0; i < MAX_TIMERS; i++)
			if(g_timers[i] == gg)
				g_timers[i] = NULL;
		gg->timer = 0;
#ifdef DEBUGMODE
		gg_netlog("gg_destroykeepalive(): Killed Timer");
#endif
	}
#ifdef DEBUGMODE
	gg_netlog("gg_destroykeepalive(): End");
#endif
}
