/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project,
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
#include "clist.h"
#include "m_genmenu.h"
#include "m_clui.h"
#pragma hdrstop

int CloseAction(WPARAM wParam,LPARAM lParam)
{
	if (CallService(MS_SYSTEM_OKTOEXIT,(WPARAM)0,(LPARAM)0))
		SendMessage( pcli->hwndContactList,WM_DESTROY,0,0 );

	return(0);
}

int InitCustomMenus(void)
{
	CreateServiceFunction("CloseAction",CloseAction);
	return 0;
}

void UninitCustomMenus(void)
{
}
