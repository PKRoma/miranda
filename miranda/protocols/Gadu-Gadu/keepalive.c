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

static int hTimer = 0;

static void CALLBACK gg_keepalive(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
	if (gg_isonline())
	{
#ifdef DEBUGMODE
		gg_netlog("Sending keep-alive");
#endif
		gg_ping(ggThread->sess);
	}
}

void gg_keepalive_init()
{
	if (DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_KEEPALIVE, GG_KEYDEF_KEEPALIVE)) {
		hTimer = SetTimer(NULL, 0, 1000 * 60, gg_keepalive);
	}
}

void gg_keepalive_destroy()
{
#ifdef DEBUGMODE
	gg_netlog("gg_destroykeepalive(): Killing Timer");
#endif
	if (hTimer) {
		KillTimer(NULL, hTimer);
#ifdef DEBUGMODE
		gg_netlog("gg_destroykeepalive(): Killed Timer");
#endif
	}
#ifdef DEBUGMODE
	gg_netlog("gg_destroykeepalive(): End");
#endif
}
