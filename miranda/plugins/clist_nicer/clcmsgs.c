/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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

UNICODE done

*/
#include "commonheaders.h"

extern struct CluiData g_CluiData;

extern struct ExtraCache *g_ExtraCache;
extern int g_nextExtraCacheEntry, g_maxExtraCacheEntry;

//processing of all the CLM_ messages incoming

extern LRESULT ( *saveProcessExternalMessages )(HWND hwnd, struct ClcData *dat, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT ProcessExternalMessages(HWND hwnd, struct ClcData *dat, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
        case CLM_SETSTICKY:
            {
                struct ClcContact *contact;
				struct ClcGroup *group;

                if (wParam == 0 || !FindItem(hwnd, dat, (HANDLE) wParam, &contact, &group, NULL))
                    return 0;
				if (lParam) {
                    contact->flags |= CONTACTF_STICKY;
					if(group && g_CluiData.bAutoExpandGroups) {
						WORD old_expanded = group->expanded & 0x0000ffff;
						int iIndex;

						group->expanded = MAKELONG(LOWORD(group->expanded), old_expanded);
						SetGroupExpand(hwnd, dat, group, 1);
						pcli->pfnEnsureVisible(hwnd, dat, 1, 0);
						iIndex = pcli->pfnGetRowsPriorTo(&dat->list, group, li.List_IndexOf(( SortedList* )&group->cl, contact));
						pcli->pfnEnsureVisible(hwnd, dat, 1, iIndex);
					}
				}
				else {
					if(group && g_CluiData.bAutoExpandGroups)
						SetGroupExpand(hwnd, dat, group, HIWORD(group->expanded));
                    contact->flags &= ~CONTACTF_STICKY;
				}
                break;
            }

	case CLM_SETEXTRAIMAGEINT:
		{
			struct ClcContact *contact = NULL;

			if (LOWORD(lParam) >= MAXEXTRACOLUMNS)
				return 0;

			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
				return 0;
			if(contact->type != CLCIT_CONTACT || contact->bIsMeta)
				return 0;

			if(contact->extraCacheEntry >= 0 && contact->extraCacheEntry < g_nextExtraCacheEntry) {
				g_ExtraCache[contact->extraCacheEntry].iExtraImage[LOWORD(lParam)] = (BYTE)HIWORD(lParam);
				g_ExtraCache[contact->extraCacheEntry].iExtraValid = g_ExtraCache[contact->extraCacheEntry].iExtraImage[LOWORD(lParam)] != (BYTE)0xff ? (g_ExtraCache[contact->extraCacheEntry].iExtraValid | (1 << LOWORD(lParam))) : (g_ExtraCache[contact->extraCacheEntry].iExtraValid & ~(1 << LOWORD(lParam)));
				PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
		}	}
		return 0;
	case CLM_SETEXTRAIMAGEINTMETA:
		{
			HANDLE hMasterContact = 0;
			int index = -1;

			if (LOWORD(lParam) >= MAXEXTRACOLUMNS)
				return 0;

			hMasterContact = (HANDLE)DBGetContactSettingDword((HANDLE)wParam, g_CluiData.szMetaName, "Handle", 0);

			index = GetExtraCache(hMasterContact, NULL);

			if(index >= 0 && index < g_nextExtraCacheEntry) {
				g_ExtraCache[index].iExtraImage[LOWORD(lParam)] = (BYTE)HIWORD(lParam);
				g_ExtraCache[index].iExtraValid = g_ExtraCache[index].iExtraImage[LOWORD(lParam)] != (BYTE)0xff ? (g_ExtraCache[index].iExtraValid | (1 << LOWORD(lParam))) : (g_ExtraCache[index].iExtraValid & ~(1 << LOWORD(lParam)));
				PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
		}	}
		return 0;

	case CLM_GETSTATUSMSG:
		{
			struct ClcContact *contact = NULL;

			if (wParam == 0)
				return 0;

			if (!FindItem(hwnd, dat, (HANDLE)wParam, &contact, NULL, NULL))
				return 0;
			if(contact->type != CLCIT_CONTACT)
				return 0;
			if(contact->extraCacheEntry >= 0 && contact->extraCacheEntry <= g_nextExtraCacheEntry) {
				if(g_ExtraCache[contact->extraCacheEntry].bStatusMsgValid != STATUSMSG_NOTFOUND)
					return((int)g_ExtraCache[contact->extraCacheEntry].statusMsg);
		}	}
		return 0;

	case CLM_SETHIDESUBCONTACTS:
		dat->bHideSubcontacts = (BOOL)lParam;
		return 0;

	case CLM_SETEXTRAIMAGELIST:
		dat->himlExtraColumns = (HIMAGELIST) lParam;
		InvalidateRect(hwnd, NULL, FALSE);
		return 0;

	case CLM_SETFONT:
		if(HIWORD(lParam)<0 || HIWORD(lParam)>FONTID_LAST) 
			return 0;
		dat->fontInfo[HIWORD(lParam)].hFont = (HFONT)wParam;
		dat->fontInfo[HIWORD(lParam)].changed = 1;

		RowHeights_GetMaxRowHeight(dat, hwnd);

		if(LOWORD(lParam))
			InvalidateRect(hwnd,NULL,FALSE);
		return 0;

	case CLM_ISMULTISELECT:
		return dat->isMultiSelect;
	}

	return saveProcessExternalMessages(hwnd, dat, msg, wParam, lParam);
}
