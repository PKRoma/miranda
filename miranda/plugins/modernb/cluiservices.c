/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
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
#include "m_clc.h"
#include "m_clui.h"
#include "commonprototypes.h"

int CLUIUnreadEmailCountChanged(WPARAM wParam,LPARAM lParam)
{
	CallService(MS_SKINENG_INVALIDATEFRAMEIMAGE, 0 ,0);
	return 0;
}

int CLUIServices_ProtocolStatusChanged(WPARAM wParam,LPARAM lParam)
{
	CallService(MS_SKINENG_INVALIDATEFRAMEIMAGE,(WPARAM)pcli->hwndStatus,0);
	if (lParam) cliTrayIconUpdateBase((char*)lParam);
	return 0;
}

void cliCluiProtocolStatusChanged(int status,const unsigned char * proto)
{
	CLUIServices_ProtocolStatusChanged((WPARAM)status,(LPARAM)proto);
}

int SortList(WPARAM wParam,LPARAM lParam)
{
    pcli->pfnClcBroadcast( WM_TIMER,TIMERID_DELAYEDRESORTCLC,0);
    pcli->pfnClcBroadcast( INTM_SCROLLBARCHANGED,0,0);

	return 0;
}

static int MetaSupportCheck(WPARAM wParam,LPARAM lParam)
{
	return 1;
}

int CLUIServices_LoadModule(void)
{
	CreateServiceFunction(MS_CLUI_METASUPPORT,MetaSupportCheck);
	CreateServiceFunction(MS_CLUI_PROTOCOLSTATUSCHANGED,CLUIServices_ProtocolStatusChanged);
	CreateServiceFunction(MS_CLUI_SORTLIST,SortList);
	return 0;
}
