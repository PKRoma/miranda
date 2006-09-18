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
*/
#include "commonheaders.h"
#include "m_clui.h"
#include "clist.h"
#include "cluiframes/cluiframes.h"


static HWND hwndEventFrame = 0;
extern struct CListEvent* ( *saveAddEvent )(CLISTEVENT *cle);
extern int ( *saveRemoveEvent )(HANDLE hContact, HANDLE hDbEvent);
extern wndFrame *wndFrameEventArea;
HWND g_hwndEventArea = 0;

void TrayIconSetToBase(char *szPreferredProto);

/*struct CListEvent {
	int imlIconIndex;
	int flashesDone;
	CLISTEVENT cle;
};
*/
struct CListEvent {
	int imlIconIndex;
	int flashesDone;
	CLISTEVENT cle;

	int menuId;
	int imlIconOverlayIndex;
};

static struct CListEvent *event;
static int eventCount;
static int disableTrayFlash;
static int disableIconFlash;


struct CListImlIcon {
	int index;
	HICON hIcon;
};

static struct CListImlIcon *imlIcon;
static int imlIconCount;
static UINT flashTimerId;
static int iconsOn;
extern HIMAGELIST hCListImages;

struct NotifyMenuItemExData {
	HANDLE hContact;
	int iIcon;              // icon index in the image list
	HICON hIcon;            // corresponding icon handle
	HANDLE hDbEvent;
};

static CLISTEVENT* MyGetEvent(int iSelection)
{
	int i;

	for (i = 0; i < pcli->events.count; i++) {
		struct CListEvent* p = pcli->events.items[i];
		if (p->menuId == iSelection)
			return &p->cle;
	}
	return NULL;
}

struct CListEvent* cli_AddEvent(CLISTEVENT *cle)
{
	struct CListEvent* p = saveAddEvent(cle);
		if ( p == NULL )
		return NULL;

//	if (1) {
//		if (p->cle.hContact != 0 && p->cle.hDbEvent != (HANDLE) 1 && !(p->cle.flags & CLEF_ONLYAFEW)) {
//			int j;
//			struct NotifyMenuItemExData *nmi = 0;
//			char *szProto;
//			TCHAR *szName;
//			MENUITEMINFO mii = {0};
//			mii.cbSize = sizeof(mii);
//			mii.fMask = MIIM_DATA | MIIM_BITMAP | MIIM_ID;
//			if (p->cle.pszService && !strncmp("SRMsg/ReadMessage", p->cle.pszService, 17)) {
//				// dup check only for msg events
//				for (j = 0; j < GetMenuItemCount(g_CluiData.hMenuNotify); j++) {
//					if (GetMenuItemInfo(g_CluiData.hMenuNotify, j, TRUE, &mii) != 0) {
//						nmi = (struct NotifyMenuItemExData *) mii.dwItemData;
//						if (nmi != 0 && (HANDLE) nmi->hContact == (HANDLE) p->cle.hContact && nmi->iIcon == p->imlIconIndex)
//							return p;
//			}	}	}
//
//			szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) p->cle.hContact, 0);
//			szName = pcli->pfnGetContactDisplayName(p->cle.hContact, 0);
//			if (szProto && szName) {
//				nmi = (struct NotifyMenuItemExData *) malloc(sizeof(struct NotifyMenuItemExData));
//				if (nmi) {
//					TCHAR szBuffer[128];
//					TCHAR* szStatus = pcli->pfnGetStatusModeDescription(DBGetContactSettingWord(p->cle.hContact, szProto, "Status", ID_STATUS_OFFLINE), 0);
//#if defined(_UNICODE)
//					TCHAR szwProto[64];
//					MultiByteToWideChar(CP_ACP, 0, szProto, -1, szwProto, 64);
//					szwProto[63] = 0;
//					_snwprintf(szBuffer, SIZEOF(szBuffer), L"%s: %s (%s)", szwProto, szName, szStatus);
//#else
//					_snprintf(szBuffer, SIZEOF(szBuffer), "%s: %s (%s)", szProto, szName, szStatus);
//#endif
//					szBuffer[127] = 0;
//					AppendMenu(g_CluiData.hMenuNotify, MF_BYCOMMAND | MF_STRING, g_CluiData.wNextMenuID, szBuffer);
//					mii.hbmpItem = HBMMENU_CALLBACK;
//					nmi->hContact = p->cle.hContact;
//					nmi->iIcon = p->imlIconIndex;
//					nmi->hIcon = p->cle.hIcon;
//					nmi->hDbEvent = p->cle.hDbEvent;
//					mii.dwItemData = (ULONG) nmi;
//					mii.wID = g_CluiData.wNextMenuID;
//					SetMenuItemInfo(g_CluiData.hMenuNotify, g_CluiData.wNextMenuID, FALSE, &mii);
//					p-> menuId = g_CluiData.wNextMenuID;
//					g_CluiData.wNextMenuID++;
//					if (g_CluiData.wNextMenuID > 0x7fff)
//						g_CluiData.wNextMenuID = 1;
//					g_CluiData.iIconNotify = p->imlIconIndex;
//				}
//			}
//		} 
//
//		if (pcli->events.count > 0) {
//			g_CluiData.bEventAreaEnabled = TRUE;
//			if (g_CluiData.notifyActive == 0) {
//				g_CluiData.notifyActive = 1;
//				HideShowNotifyFrame();
//			}
//		}
//		InvalidateRect(hwndEventFrame, NULL, FALSE);
//	}
	
	return p;
}


int cli_RemoveEvent(HANDLE hContact, HANDLE hDbEvent)
{
	int res=saveRemoveEvent(hContact, hDbEvent);
	return res;
}