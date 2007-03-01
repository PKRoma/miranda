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
#include "m_clc.h"
#include "clc.h"
#include "commonprototypes.h"


//processing of all the CLM_ messages incoming

LRESULT cli_ProcessExternalMessages(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg) 
	{

	case CLM_AUTOREBUILD:
		if (dat->force_in_dialog)
		{
			pcli->pfnSaveStateAndRebuildList(hwnd, dat);
		}
		else
		{
			KillTimer(hwnd,TIMERID_REBUILDAFTER);
			// Give some time to gather other events too		
			CLUI_SafeSetTimer(hwnd,TIMERID_REBUILDAFTER,50,NULL);
			CLM_AUTOREBUILD_WAS_POSTED=FALSE;
		}
		return 0;

	case CLM_SETEXTRACOLUMNSSPACE:
		dat->extraColumnSpacing=(int)wParam;
		CLUI__cliInvalidateRect(hwnd,NULL,FALSE);
		return 0;

	case CLM_SETFONT:
		if(HIWORD(lParam)<0 || HIWORD(lParam)>FONTID_MODERN_MAX) return 0;

		dat->fontModernInfo[HIWORD(lParam)].hFont=(HFONT)wParam;
		dat->fontModernInfo[HIWORD(lParam)].changed=1;

		RowHeights_GetMaxRowHeight(dat, hwnd);

		if(LOWORD(lParam))
			CLUI__cliInvalidateRect(hwnd,NULL,FALSE);
		return 0;


	case CLM_SETHIDEEMPTYGROUPS:
		{
			BOOL old=((GetWindowLong(hwnd,GWL_STYLE)&CLS_HIDEEMPTYGROUPS)!=0);
			BOOL newval=old;
			if(wParam) SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)|CLS_HIDEEMPTYGROUPS);
			else SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)&~CLS_HIDEEMPTYGROUPS);
			newval=((GetWindowLong(hwnd,GWL_STYLE)&CLS_HIDEEMPTYGROUPS)!=0);
			if (newval!=old)
				SendMessage(hwnd,CLM_AUTOREBUILD,0,0);
		}
		return 0;

	case CLM_SETTEXTCOLOR:
		if(wParam<0 || wParam>FONTID_MODERN_MAX) break;

		dat->fontModernInfo[wParam].colour=lParam;
		dat->force_in_dialog=TRUE;
		if (wParam==FONTID_CONTACTS) {
			dat->fontModernInfo[FONTID_SECONDLINE].colour=lParam;
			dat->fontModernInfo[FONTID_THIRDLINE].colour=lParam;
		}

	case CLM_GETNEXTITEM:
		{
			struct ClcContact *contact;
			struct ClcGroup *group;
			int i;

			if (wParam != CLGN_ROOT) {
				if (!pcli->pfnFindItem(hwnd, dat, (HANDLE) lParam, &contact, &group, NULL))
					return (LRESULT) (HANDLE) NULL;
				i = li.List_IndexOf((SortedList*)&group->cl,contact);
				if (i<0) return 0;
			}
			switch (wParam) 
			{
			case CLGN_ROOT:
				if (dat->list.cl.count)
					return (LRESULT) pcli->pfnContactToHItem(dat->list.cl.items[0]);
				else
					return (LRESULT) (HANDLE) NULL;
			case CLGN_CHILD:
				if (contact->type != CLCIT_GROUP)
					return (LRESULT) (HANDLE) NULL;
				group = contact->group;
				if (group->cl.count == 0)
					return (LRESULT) (HANDLE) NULL;
				return (LRESULT) pcli->pfnContactToHItem(group->cl.items[0]);
			case CLGN_PARENT:
				return group->groupId | HCONTACT_ISGROUP;
			case CLGN_NEXT:
				if (i >= group->cl.count)
					return (LRESULT) (HANDLE) NULL;
				return (LRESULT) pcli->pfnContactToHItem(group->cl.items[i + 1]);
			case CLGN_PREVIOUS:
				if (i <= 0)
					return (LRESULT) (HANDLE) NULL;
				return (LRESULT) pcli->pfnContactToHItem(group->cl.items[i - 1]);
			case CLGN_NEXTCONTACT:
				for (i++; i < group->cl.count; i++)
					if (group->cl.items[i]->type == CLCIT_CONTACT)
						break;
				if (i >= group->cl.count)
					return (LRESULT) (HANDLE) NULL;
				return (LRESULT) pcli->pfnContactToHItem(group->cl.items[i]);
			case CLGN_PREVIOUSCONTACT:
				if (i >= group->cl.count)
					return (LRESULT) (HANDLE) NULL;
				for (i--; i >= 0; i--)
					if (group->cl.items[i]->type == CLCIT_CONTACT)
						break;
				if (i < 0)
					return (LRESULT) (HANDLE) NULL;
				return (LRESULT) pcli->pfnContactToHItem(group->cl.items[i]);
			case CLGN_NEXTGROUP:
				for (i++; i < group->cl.count; i++)
					if (group->cl.items[i]->type == CLCIT_GROUP)
						break;
				if (i >= group->cl.count)
					return (LRESULT) (HANDLE) NULL;
				return (LRESULT) pcli->pfnContactToHItem(group->cl.items[i]);
			case CLGN_PREVIOUSGROUP:
				if (i >= group->cl.count)
					return (LRESULT) (HANDLE) NULL;
				for (i--; i >= 0; i--)
					if (group->cl.items[i]->type == CLCIT_GROUP)
						break;
				if (i < 0)
					return (LRESULT) (HANDLE) NULL;
				return (LRESULT) pcli->pfnContactToHItem(group->cl.items[i]);
			}
			return (LRESULT) (HANDLE) NULL;
		}

		return 0;
	}

	return saveProcessExternalMessages(hwnd, dat, msg, wParam, lParam);
}
