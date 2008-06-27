/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

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
#include "resource.h"
#include "m_userinfo.h"
#include "cluiframes/cluiframes.h"
#include "coolsb/coolscroll.h"

int DefaultImageListColorDepth = ILC_COLOR32;

extern CRITICAL_SECTION cs_extcache;
extern struct CluiData g_CluiData;
extern struct ClcData *g_clcData;
extern HPEN g_hPenCLUIFrames;
extern HANDLE hExtraImageApplying;
extern wndFrame *wndFrameCLC;
extern ButtonItem *g_ButtonItems;

extern pfnDrawAlpha pDrawAlpha;
extern BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);
extern int during_sizing;
extern StatusItems_t *StatusItems;
extern int g_shutDown;
extern int g_nextExtraCacheEntry;
extern struct ExtraCache *g_ExtraCache;

HIMAGELIST hCListImages;
extern HIMAGELIST himlExtraImages;

HANDLE hSoundHook = 0, hIcoLibChanged = 0, hSvc_GetContactStatusMsg = 0;

static HANDLE hClcSettingsChanged, hClcDBEvent = 0;

static HRESULT(WINAPI *MyCloseThemeData)(HANDLE);

LONG g_cxsmIcon, g_cysmIcon;

void  SetDBButtonStates(HANDLE hContact);

HMENU BuildGroupPopupMenu(struct ClcGroup* group)
{
	return (HMENU)CallService(MS_CLIST_MENUBUILDSUBGROUP, (WPARAM)group, 0);
}

int AvatarChanged(WPARAM wParam, LPARAM lParam)
{
	pcli->pfnClcBroadcast(INTM_AVATARCHANGED, wParam, lParam);
	return 0;
}

int __forceinline __strcmp(const char * src, const char * dst)
{
	int ret = 0 ;

	while (!(ret = *(unsigned char *)src - *(unsigned char *)dst) && *dst)
		++src, ++dst;
	return(ret);
}

static int ClcEventAdded(WPARAM wParam, LPARAM lParam)
{
	DBEVENTINFO dbei = {0};
	int iEntry;
	DWORD new_freq = 0;

	g_CluiData.t_now = time(NULL);

	if (wParam && lParam) {
		dbei.cbSize = sizeof(dbei);
		dbei.pBlob = 0;
		dbei.cbBlob = 0;
		CallService(MS_DB_EVENT_GET, (WPARAM)lParam, (LPARAM)&dbei);
		if (dbei.eventType == EVENTTYPE_MESSAGE && !(dbei.flags & DBEF_SENT)) {
			DWORD firstTime = DBGetContactSettingDword((HANDLE)wParam, "CList", "mf_firstEvent", 0);
			DWORD count = DBGetContactSettingDword((HANDLE)wParam, "CList", "mf_count", 0);
			count++;
			new_freq = count ? (dbei.timestamp - firstTime) / count : 0x7fffffff;
			DBWriteContactSettingDword((HANDLE)wParam, "CList", "mf_freq", new_freq);
			DBWriteContactSettingDword((HANDLE)wParam, "CList", "mf_count", count);
			iEntry = GetExtraCache((HANDLE)wParam, NULL);
			if (iEntry >= 0 && iEntry < g_nextExtraCacheEntry) {
				g_ExtraCache[iEntry].dwLastMsgTime = dbei.timestamp;
				if (new_freq)
					g_ExtraCache[iEntry].msgFrequency = new_freq;
				pcli->pfnClcBroadcast(INTM_FORCESORT, 0, 1);
			}
		}
	}
	return 0;
}

int ClcSoundHook(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

static int ClcSettingChanged(WPARAM wParam, LPARAM lParam)
{
	char *szProto = NULL;
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;

	if (wParam) {
		if (!__strcmp(cws->szModule, "CList")) {
			if (!__strcmp(cws->szSetting, "StatusMsg"))
				SendMessage(pcli->hwndContactTree, INTM_STATUSMSGCHANGED, wParam, lParam);
		} else if (!__strcmp(cws->szModule, "UserInfo")) {
			if (!__strcmp(cws->szSetting, "ANSIcodepage"))
				pcli->pfnClcBroadcast(INTM_CODEPAGECHANGED, wParam, lParam);
			else if (!__strcmp(cws->szSetting, "Timezone"))
				ReloadExtraInfo((HANDLE)wParam);
		} else if (wParam != 0 && (szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0)) != NULL) {
			char *id = NULL;
			if (!__strcmp(cws->szModule, "Protocol") && !__strcmp(cws->szSetting, "p")) {
				char *szProto_s;
				pcli->pfnClcBroadcast(INTM_PROTOCHANGED, wParam, lParam);
				if (cws->value.type == DBVT_DELETED)
					szProto_s = NULL;
				else
					szProto_s = cws->value.pszVal;
				pcli->pfnChangeContactIcon((HANDLE) wParam, IconFromStatusMode(szProto_s, szProto_s == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord((HANDLE) wParam, szProto_s, "Status", ID_STATUS_OFFLINE), (HANDLE) wParam, NULL), 0);
			}
			// something is being written to a protocol module
			if (!__strcmp(szProto, cws->szModule)) {
				// was a unique setting key written?
				pcli->pfnInvalidateDisplayNameCacheEntry((HANDLE) wParam);
				if (!__strcmp(cws->szSetting, "Status")) {
					if (!DBGetContactSettingByte((HANDLE) wParam, "CList", "Hidden", 0)) {
						if (DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT)) {
							// User's state is changing, and we are hideOffline-ing
							if (cws->value.wVal == ID_STATUS_OFFLINE) {
								pcli->pfnChangeContactIcon((HANDLE) wParam, IconFromStatusMode(cws->szModule, cws->value.wVal, (HANDLE) wParam, NULL), 0);
								CallService(MS_CLUI_CONTACTDELETED, wParam, 0);
								return 0;
							}
							pcli->pfnChangeContactIcon((HANDLE) wParam, IconFromStatusMode(cws->szModule, cws->value.wVal, (HANDLE) wParam, NULL), 1);
						}
						pcli->pfnChangeContactIcon((HANDLE) wParam, IconFromStatusMode(cws->szModule, cws->value.wVal, (HANDLE) wParam, NULL), 0);
					}
					SendMessage(pcli->hwndContactTree, INTM_STATUSCHANGED, wParam, lParam);
					return 0;
				} else if (strstr("YMsg|StatusDescr|XStatusMsg", cws->szSetting))
					SendMessage(pcli->hwndContactTree, INTM_STATUSMSGCHANGED, wParam, lParam);
				else if (strstr(cws->szSetting, "XStatus"))
					SendMessage(pcli->hwndContactTree, INTM_XSTATUSCHANGED, wParam, lParam);
				else if (!__strcmp(cws->szSetting, "Timezone"))
					ReloadExtraInfo((HANDLE)wParam);
				else if (!__strcmp(cws->szSetting, "MirVer"))
					NotifyEventHooks(hExtraImageApplying, wParam, 0);

				if (g_CluiData.bMetaAvail && !(g_CluiData.dwFlags & CLUI_USEMETAICONS) && !__strcmp(szProto, g_CluiData.szMetaName)) {
					if ((lstrlenA(cws->szSetting) > 6 && !strncmp(cws->szSetting, "Status", 6)) || strstr("Default,ForceSend,Nick", cws->szSetting))
						pcli->pfnClcBroadcast(INTM_NAMEORDERCHANGED, wParam, lParam);
				}
			}
			if (g_CluiData.bMetaAvail && g_CluiData.bMetaEnabled && !__strcmp(cws->szModule, g_CluiData.szMetaName) && !__strcmp(cws->szSetting, "IsSubcontact"))
				pcli->pfnClcBroadcast(INTM_HIDDENCHANGED, wParam, lParam);
		}
	} else if (wParam == 0 && !__strcmp(cws->szModule, g_CluiData.szMetaName)) {
		BYTE bMetaEnabled = DBGetContactSettingByte(NULL, g_CluiData.szMetaName, "Enabled", 1);
		if (bMetaEnabled != (BYTE)g_CluiData.bMetaEnabled) {
			g_CluiData.bMetaEnabled = bMetaEnabled;
			pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
		}
	} else if (wParam == 0 && !__strcmp(cws->szModule, "Skin")) {
		if (!__strcmp(cws->szSetting, "UseSound")) {
			if (hSoundHook) {
				UnhookEvent(hSoundHook);
				hSoundHook = 0;
			}
			g_CluiData.soundsOff = DBGetContactSettingByte(0, cws->szModule, cws->szSetting, 0) ? 0 : 1;
			if (g_CluiData.soundsOff && hSoundHook == 0)
				hSoundHook = HookEvent(ME_SKIN_PLAYINGSOUND, ClcSoundHook);
			CheckDlgButton(pcli->hwndContactList, IDC_TBSOUND, g_CluiData.soundsOff ? BST_UNCHECKED : BST_CHECKED);
			SetButtonStates(pcli->hwndContactList);
		}
	} else if (szProto == NULL && wParam == 0) {
		if (!__strcmp(cws->szSetting, "XStatusId"))
			CluiProtocolStatusChanged(0, cws->szModule);
		return 0;
	}
	return 0;
}

static int ClcModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

static int ClcPreshutdown(WPARAM wParam, LPARAM lParam)
{
	SFL_Destroy();
	g_shutDown = TRUE;
	if (hSvc_GetContactStatusMsg)
		DestroyServiceFunction(hSvc_GetContactStatusMsg);
	UnhookEvent(hClcSettingsChanged);
	UnhookEvent(hClcDBEvent);
	if (hIcoLibChanged)
		UnhookEvent(hIcoLibChanged);
	return 0;
}

int ClcShutdown(WPARAM wParam, LPARAM lParam)
{
	if (g_CluiData.hIconInvisible)
		DestroyIcon(g_CluiData.hIconInvisible);
	if (g_CluiData.hIconVisible)
		DestroyIcon(g_CluiData.hIconVisible);
	if (g_CluiData.hIconChatactive)
		DestroyIcon(g_CluiData.hIconChatactive);

	DeleteObject(g_CluiData.hPen3DBright);
	DeleteObject(g_CluiData.hPen3DDark);
	DeleteObject(g_CluiData.hBrushColorKey);
	DeleteObject(g_CluiData.hBrushCLCBk);
	DeleteObject(g_CluiData.hBrushAvatarBorder);
	ClearIcons(1);
	pDrawAlpha = 0;
	SFL_UnregisterWindowClass();
	if (g_ExtraCache) {
		int i;

		for (i = 0; i < g_nextExtraCacheEntry; i++) {
			if (g_ExtraCache[i].statusMsg)
				free(g_ExtraCache[i].statusMsg);
			if (g_ExtraCache[i].status_item) {
				StatusItems_t *item = g_ExtraCache[i].status_item;
				int j;

				free(g_ExtraCache[i].status_item);
				g_ExtraCache[i].status_item = 0;
				for (j = i; j < g_nextExtraCacheEntry; j++) {			// avoid duplicate free()'ing status item pointers (there are references from sub to master contacts, so compare the pointers...
					if (g_ExtraCache[j].status_item == item)
						g_ExtraCache[j].status_item = 0;
				}
			}
		}
		free(g_ExtraCache);
		g_ExtraCache = NULL;
	}
	IMG_DeleteItems();
	free(StatusItems);
	DeleteCriticalSection(&cs_extcache);
	return 0;
}

int LoadCLCModule(void)
{
	g_cxsmIcon = GetSystemMetrics(SM_CXSMICON);
	g_cysmIcon = GetSystemMetrics(SM_CYSMICON);

	hCListImages = (HIMAGELIST) CallService(MS_CLIST_GETICONSIMAGELIST, 0, 0);

	HookEvent(ME_SYSTEM_MODULESLOADED, ClcModulesLoaded);
	hClcSettingsChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ClcSettingChanged);
	hClcDBEvent = HookEvent(ME_DB_EVENT_ADDED, ClcEventAdded);
	HookEvent(ME_OPT_INITIALISE, ClcOptInit);
	HookEvent(ME_SYSTEM_SHUTDOWN, ClcPreshutdown);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// clist_nicer+ control window procedure

extern LRESULT(CALLBACK *saveContactListControlWndProc)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK ContactListControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct ClcData *dat;
	BOOL   frameHasTitlebar = FALSE;

	if (wndFrameCLC)
		frameHasTitlebar = wndFrameCLC->TitleBar.ShowTitleBar;

	dat = (struct ClcData *) GetWindowLong(hwnd, 0);
	if (msg >= CLM_FIRST && msg < CLM_LAST)
		return ProcessExternalMessages(hwnd, dat, msg, wParam, lParam);

	switch (msg) {
		case WM_CREATE:
			dat = (struct ClcData *)mir_alloc(sizeof(struct ClcData));
			memset(dat, 0, sizeof(struct ClcData));
			SetWindowLong(hwnd, 0, (LONG) dat);

			RowHeights_Initialize(dat);
			dat->forceScroll = 0;
			dat->lastRepaint = 0;
			dat->himlExtraColumns = himlExtraImages;
			dat->hwndParent = GetParent(hwnd);
			dat->lastSort = GetTickCount();
			dat->bNeedSort = FALSE;
			{
				CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
				if (cs->lpCreateParams == (LPVOID)0xff00ff00) {
					dat->bisEmbedded = FALSE;
					dat->bHideSubcontacts = TRUE;
					g_clcData = dat;
					if (g_CluiData.bShowLocalTime)
						SetTimer(hwnd, TIMERID_REFRESH, 65000, NULL);
				} else
					dat->bisEmbedded = TRUE;
			}
			break;
		case WM_SIZE:
			pcli->pfnEndRename(hwnd, dat, 1);
			KillTimer(hwnd, TIMERID_INFOTIP);
			KillTimer(hwnd, TIMERID_RENAME);
			pcli->pfnRecalcScrollBar(hwnd, dat);
LBL_Def:
			return DefWindowProc(hwnd, msg, wParam, lParam);

		case WM_NCCALCSIZE: {
			return FrameNCCalcSize(hwnd, DefWindowProc, wParam, lParam, frameHasTitlebar);
		}

		/*
		 * scroll bar handling
		*/
		case WM_NCPAINT:
			return FrameNCPaint(hwnd, DefWindowProc, wParam, lParam, frameHasTitlebar);
		case INTM_GROUPCHANGED: {
			struct ClcContact *contact;
			BYTE iExtraImage[MAXEXTRACOLUMNS];
			BYTE flags = 0;
			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
				memset(iExtraImage, 0xFF, sizeof(iExtraImage));
			else {
				CopyMemory(iExtraImage, contact->iExtraImage, sizeof(iExtraImage));
				flags = contact->flags;
			}
			pcli->pfnDeleteItemFromTree(hwnd, (HANDLE) wParam);
			if (GetWindowLong(hwnd, GWL_STYLE) & CLS_SHOWHIDDEN || !CLVM_GetContactHiddenStatus((HANDLE)wParam, NULL, dat)) {
				NMCLISTCONTROL nm;
				pcli->pfnAddContactToTree(hwnd, dat, (HANDLE) wParam, 1, 1);
				if (FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL)) {
					CopyMemory(contact->iExtraImage, iExtraImage, sizeof(iExtraImage));
					if (flags & CONTACTF_CHECKED)
						contact->flags |= CONTACTF_CHECKED;
				}
				nm.hdr.code = CLN_CONTACTMOVED;
				nm.hdr.hwndFrom = hwnd;
				nm.hdr.idFrom = GetDlgCtrlID(hwnd);
				nm.flags = 0;
				nm.hItem = (HANDLE) wParam;
				SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm);
			}
			dat->bNeedSort = TRUE;
			PostMessage(hwnd, INTM_SORTCLC, 0, 1);
			goto LBL_Def;
		}

		case INTM_ICONCHANGED: {
			struct ClcContact *contact = NULL;
			struct ClcGroup *group = NULL;
			int recalcScrollBar = 0, shouldShow;
			WORD status = ID_STATUS_OFFLINE;
			char *szProto;
			int  contactRemoved = 0;
			HANDLE hSelItem = NULL;
			struct ClcContact *selcontact = NULL;

			szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
			if (szProto == NULL)
				status = ID_STATUS_OFFLINE;
			else
				status = DBGetContactSettingWord((HANDLE) wParam, szProto, "Status", ID_STATUS_OFFLINE);

			shouldShow = (GetWindowLong(hwnd, GWL_STYLE) & CLS_SHOWHIDDEN || !CLVM_GetContactHiddenStatus((HANDLE)wParam, szProto, dat)) && ((g_CluiData.bFilterEffective ? TRUE : !pcli->pfnIsHiddenMode(dat, status)) || CallService(MS_CLIST_GETCONTACTICON, wParam, 0) != lParam);// XXX CLVM changed - this means an offline msg is flashing, so the contact should be shown
			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, &group, NULL)) {
				if (shouldShow && CallService(MS_DB_CONTACT_IS, wParam, 0)) {
					if (dat->selection >= 0 && pcli->pfnGetRowByIndex(dat, dat->selection, &selcontact, NULL) != -1)
						hSelItem = pcli->pfnContactToHItem(selcontact);
					pcli->pfnAddContactToTree(hwnd, dat, (HANDLE) wParam, 0, 0);
					recalcScrollBar = 1;
					FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL);
					if (contact) {
						contact->iImage = (WORD) lParam;
						pcli->pfnNotifyNewContact(hwnd, (HANDLE) wParam);
					}
				}
			} else {
				//item in list already
				DWORD style = GetWindowLong(hwnd, GWL_STYLE);
				if (contact->iImage == (WORD) lParam)
					break;
				if (!shouldShow && !(style & CLS_NOHIDEOFFLINE) && (style & CLS_HIDEOFFLINE || group->hideOffline || g_CluiData.bFilterEffective)) {        // CLVM changed
					if (dat->selection >= 0 && pcli->pfnGetRowByIndex(dat, dat->selection, &selcontact, NULL) != -1)
						hSelItem = pcli->pfnContactToHItem(selcontact);
					pcli->pfnRemoveItemFromGroup(hwnd, group, contact, 0);
					contactRemoved = TRUE;
					recalcScrollBar = 1;
				} else {
					contact->iImage = (WORD) lParam;
					if (!pcli->pfnIsHiddenMode(dat, status))
						contact->flags |= CONTACTF_ONLINE;
					else
						contact->flags &= ~CONTACTF_ONLINE;
				}
			}
			if (hSelItem) {
				struct ClcGroup *selgroup;
				if (pcli->pfnFindItem(hwnd, dat, hSelItem, &selcontact, &selgroup, NULL))
					dat->selection = pcli->pfnGetRowsPriorTo(&dat->list, selgroup, li.List_IndexOf((SortedList*) & selgroup->cl, selcontact));
				else
					dat->selection = -1;
			}
			dat->bNeedSort = TRUE;
			PostMessage(hwnd, INTM_SORTCLC, 0, recalcScrollBar);
			PostMessage(hwnd, INTM_INVALIDATE, 0, (LPARAM)(contactRemoved ? 0 : wParam));
			if (recalcScrollBar)
				pcli->pfnRecalcScrollBar(hwnd, dat);
			goto LBL_Def;
		}
		case INTM_METACHANGED: {
			struct ClcContact *contact;
			if (!pcli->pfnFindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
				break;
			if (contact->bIsMeta && g_CluiData.bMetaAvail && !(g_CluiData.dwFlags & CLUI_USEMETAICONS)) {
				contact->hSubContact = (HANDLE) CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM) contact->hContact, 0);
				contact->metaProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) contact->hSubContact, 0);
				contact->iImage = CallService(MS_CLIST_GETCONTACTICON, (WPARAM) contact->hSubContact, 0);
				if (contact->extraCacheEntry >= 0 && contact->extraCacheEntry < g_nextExtraCacheEntry) {
					int subIndex = GetExtraCache(contact->hSubContact, contact->metaProto);
					g_ExtraCache[contact->extraCacheEntry].proto_status_item = GetProtocolStatusItem(contact->metaProto);
					if (subIndex >= 0 && subIndex <= g_nextExtraCacheEntry) {
						g_ExtraCache[contact->extraCacheEntry].status_item = g_ExtraCache[subIndex].status_item;
						CopyMemory(g_ExtraCache[contact->extraCacheEntry].iExtraImage, g_ExtraCache[subIndex].iExtraImage, MAXEXTRACOLUMNS);
						g_ExtraCache[contact->extraCacheEntry].iExtraValid = g_ExtraCache[subIndex].iExtraValid;
					}
				}
			}
			SendMessage(hwnd, INTM_NAMEORDERCHANGED, wParam, lParam);
			goto LBL_Def;
		}
		case INTM_METACHANGEDEVENT: {
			struct ClcContact *contact;
			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
				break;
			if (lParam == 0)
				SendMessage(hwnd, CLM_AUTOREBUILD, 0, 0);
			goto LBL_Def;
		}
		case INTM_NAMECHANGED: {
			struct ClcContact *contact;
			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
				break;
			lstrcpyn(contact->szText, pcli->pfnGetContactDisplayName((HANDLE)wParam, 0), safe_sizeof(contact->szText));
#if defined(_UNICODE)
			RTL_DetectAndSet(contact, 0);
#endif
			dat->bNeedSort = TRUE;
			PostMessage(hwnd, INTM_SORTCLC, 0, 0);
			goto LBL_Def;
		}

		case INTM_CODEPAGECHANGED: {
			struct ClcContact *contact = NULL;
			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
				break;
			contact->codePage = DBGetContactSettingDword((HANDLE) wParam, "Tab_SRMsg", "ANSIcodepage", DBGetContactSettingDword((HANDLE)wParam, "UserInfo", "ANSIcodepage", CP_ACP));
			PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
			goto LBL_Def;
		}
		case INTM_AVATARCHANGED: {
			struct avatarCacheEntry *cEntry = (struct avatarCacheEntry *)lParam;
			struct ClcContact *contact = NULL;

			if (wParam == 0) {
				//RemoveFromImgCache(0, cEntry);
				g_CluiData.bForceRefetchOnPaint = TRUE;
				RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
				g_CluiData.bForceRefetchOnPaint = FALSE;
				goto LBL_Def;
			}

			if (!FindItem(hwnd, dat, (HANDLE)wParam, &contact, NULL, NULL))
				return 0;
			contact->ace = cEntry;
			if (cEntry == NULL)
				contact->cFlags &= ~ECF_AVATAR;
			else {
				DWORD dwFlags;

				if (contact->extraCacheEntry >= 0 && contact->extraCacheEntry < g_nextExtraCacheEntry)
					dwFlags = g_ExtraCache[contact->extraCacheEntry].dwDFlags;
				else
					dwFlags = DBGetContactSettingDword(contact->hContact, "CList", "CLN_Flags", 0);
				if (g_CluiData.dwFlags & CLUI_FRAME_AVATARS)
					contact->cFlags = (dwFlags & ECF_HIDEAVATAR ? contact->cFlags & ~ECF_AVATAR : contact->cFlags | ECF_AVATAR);
				else
					contact->cFlags = (dwFlags & ECF_FORCEAVATAR ? contact->cFlags | ECF_AVATAR : contact->cFlags & ~ECF_AVATAR);
			}
			PostMessage(hwnd, INTM_INVALIDATE, 0, (LPARAM)contact->hContact);
			goto LBL_Def;
		}
		case INTM_STATUSMSGCHANGED: {
			struct ClcContact *contact = NULL;
			int index = -1;
			char *szProto = NULL;

			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
				index = GetExtraCache((HANDLE)wParam, NULL);
			else {
				index = contact->extraCacheEntry;
				szProto = contact->proto;
			}
			GetCachedStatusMsg(index, szProto);
			PostMessage(hwnd, INTM_INVALIDATE, 0, (LPARAM)(contact ? contact->hContact : 0));
			goto LBL_Def;
		}
		case INTM_STATUSCHANGED: {
			struct ClcContact *contact = NULL;
			WORD wStatus;

			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
				break;

			wStatus = DBGetContactSettingWord((HANDLE)wParam, contact->proto, "Status", ID_STATUS_OFFLINE);
			if (g_CluiData.bNoOfflineAvatars && wStatus != ID_STATUS_OFFLINE && contact->wStatus == ID_STATUS_OFFLINE) {
				contact->wStatus = wStatus;
				if (g_CluiData.bAvatarServiceAvail && contact->ace == NULL)
					LoadAvatarForContact(contact);
			}
			contact->wStatus = wStatus;
			goto LBL_Def;
		}
		case INTM_PROTOCHANGED: {
			DBCONTACTWRITESETTING *dbcws = (DBCONTACTWRITESETTING *) lParam;
			struct ClcContact *contact = NULL;

			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
				break;
			contact->proto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
			CallService(MS_CLIST_INVALIDATEDISPLAYNAME, wParam, 0);
			lstrcpyn(contact->szText, pcli->pfnGetContactDisplayName((HANDLE)wParam, 0), safe_sizeof(contact->szText));
#if defined(_UNICODE)
			RTL_DetectAndSet(contact, 0);
#endif
			dat->bNeedSort = TRUE;
			PostMessage(hwnd, INTM_SORTCLC, 0, 0);
			goto LBL_Def;
		}

		case INTM_INVALIDATE:
			if (!dat->bNeedPaint) {
				KillTimer(hwnd, TIMERID_PAINT);
				SetTimer(hwnd, TIMERID_PAINT, 100, NULL);
				dat->bNeedPaint = TRUE;
			}

			if (lParam && !dat->bisEmbedded) {
				struct ClcContact *contact = NULL;

				if (FindItem(hwnd, dat, (HANDLE)lParam, &contact, NULL, 0)) {
					if (contact && contact->extraCacheEntry >= 0 && contact->extraCacheEntry < g_nextExtraCacheEntry && g_ExtraCache[contact->extraCacheEntry].floater)
						FLT_Update(dat, contact);
				}
			}
			goto LBL_Def;

		case INTM_INVALIDATECONTACT: {
			struct ClcContact *contact = 0;
			struct ClcGroup *group = 0;
			int iItem;

			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, &group, NULL))
				break;

			if (contact == 0 || group == 0)
				break;

			iItem = pcli->pfnGetRowsPriorTo(&dat->list, group, li.List_IndexOf((SortedList*) & group->cl, contact));
			pcli->pfnInvalidateItem(hwnd, dat, iItem);
			goto LBL_Def;
		}
		case INTM_FORCESORT:
			dat->bNeedSort = TRUE;
			return SendMessage(hwnd, INTM_SORTCLC, wParam, lParam);
		case INTM_SORTCLC:
			if (dat->bNeedSort) {
				pcli->pfnSortCLC(hwnd, dat, TRUE);
				dat->bNeedSort = FALSE;
			}
			if (lParam)
				pcli->pfnRecalcScrollBar(hwnd, dat);
			goto LBL_Def;

		case INTM_IDLECHANGED: {
			DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;
			char *szProto;
			struct ClcContact *contact = NULL;

			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL))
				break;
			szProto = (char*)cws->szModule;
			if (szProto == NULL)
				break;
			contact->flags &= ~CONTACTF_IDLE;
			if (DBGetContactSettingDword((HANDLE) wParam, szProto, "IdleTS", 0)) {
				contact->flags |= CONTACTF_IDLE;
			}
			PostMessage(hwnd, INTM_INVALIDATE, 0, (LPARAM)contact->hContact);
			goto LBL_Def;
		}
		case INTM_XSTATUSCHANGED: {
			DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) lParam;
			char *szProto;
			struct ClcContact *contact = NULL;
			int index;

			szProto = (char *)cws->szModule;

			if (!FindItem(hwnd, dat, (HANDLE) wParam, &contact, NULL, NULL)) {
				index = GetExtraCache((HANDLE)wParam, szProto);
				if (!dat->bisEmbedded && g_CluiData.bMetaAvail && szProto) {				// may be a subcontact, forward the xstatus
					if (DBGetContactSettingByte((HANDLE)wParam, g_CluiData.szMetaName, "IsSubcontact", 0)) {
						HANDLE hMasterContact = (HANDLE)DBGetContactSettingDword((HANDLE)wParam, g_CluiData.szMetaName, "Handle", 0);
						if (hMasterContact && hMasterContact != (HANDLE)wParam)				// avoid recursive call of settings handler
							DBWriteContactSettingByte(hMasterContact, g_CluiData.szMetaName, "XStatusId",
													  (BYTE)DBGetContactSettingByte((HANDLE)wParam, szProto, "XStatusId", 0));
						break;
					}
				}
			} else {
				contact->xStatus = DBGetContactSettingByte((HANDLE) wParam, szProto, "XStatusId", 0);
				index = contact->extraCacheEntry;
			}
			if (szProto == NULL)
				break;
			GetCachedStatusMsg(index, szProto);
			PostMessage(hwnd, INTM_INVALIDATE, 0, (LPARAM)(contact ? contact->hContact : 0));
			goto LBL_Def;
		}
		case WM_PAINT: {
			HDC hdc;
			PAINTSTRUCT ps;
			hdc = BeginPaint(hwnd, &ps);
			if (IsWindowVisible(hwnd) && !during_sizing && !g_shutDown) {
				PaintClc(hwnd, dat, hdc, &ps.rcPaint);
				dat->bNeedPaint = FALSE;
				dat->lastRepaint = GetTickCount();
			}
			EndPaint(hwnd, &ps);
			if (dat->selection != dat->oldSelection && !dat->bisEmbedded && g_ButtonItems != NULL) {
				SetDBButtonStates(0);
				dat->oldSelection = dat->selection;
			}
			goto LBL_Def;
		}

		case WM_MOUSEWHEEL:
			dat->forceScroll = TRUE;
			break;

		case WM_TIMER:
			if (wParam == TIMERID_PAINT) {
				KillTimer(hwnd, TIMERID_PAINT);
				InvalidateRect(hwnd, NULL, FALSE);
				goto LBL_Def;
			}

			if (wParam == TIMERID_REFRESH) {
				InvalidateRect(hwnd, NULL, FALSE);
				goto LBL_Def;
			}
			break;

		case WM_LBUTTONDBLCLK: {
			struct ClcContact *contact;
			DWORD hitFlags;
			ReleaseCapture();
			dat->iHotTrack = -1;
			pcli->pfnHideInfoTip(hwnd, dat);
			KillTimer(hwnd, TIMERID_RENAME);
			KillTimer(hwnd, TIMERID_INFOTIP);
			dat->szQuickSearch[0] = 0;
			dat->selection = HitTest(hwnd, dat, (short) LOWORD(lParam), (short) HIWORD(lParam), &contact, NULL, &hitFlags);
			if (hitFlags & CLCHT_ONITEMEXTRAEX && hwnd == pcli->hwndContactTree && contact != 0) {
				int column = hitFlags >> 24;
				if (column-- > 0) {
					if (contact->type == CLCIT_CONTACT) {
						if (column == 0) {
							char buf[4096];
							DBVARIANT dbv = {0};
							char *szEmail = NULL;
							if (!DBGetContactSettingString(contact->hContact, "UserInfo", "Mye-mail0", &dbv))
								szEmail = dbv.pszVal;
							else if (!DBGetContactSettingString(contact->hContact, contact->proto, "e-mail", &dbv))
								szEmail = dbv.pszVal;

							if (szEmail) {
								mir_snprintf(buf, sizeof(buf), "mailto:%s", szEmail);
								mir_free(szEmail);
								ShellExecuteA(hwnd, "open", buf, NULL, NULL, SW_SHOW);
							}
							return TRUE;
						}
						if (column == 1) {
							char *homepage = NULL;
							DBVARIANT dbv = {0};

							if (!DBGetContactSettingString(contact->hContact, "UserInfo", "Homepage", &dbv))
								homepage = dbv.pszVal;
							else if (!DBGetContactSettingString(contact->hContact, contact->proto, "Homepage", &dbv))
								homepage = dbv.pszVal;
							if (homepage) {
								ShellExecuteA(hwnd, "open", homepage, NULL, NULL, SW_SHOW);
								mir_free(homepage);
							}
							return TRUE;
						}
					}
				}
			}
			InvalidateRect(hwnd, NULL, FALSE);
			if (dat->selection != -1)
				pcli->pfnEnsureVisible(hwnd, dat, dat->selection, 0);
			if (hitFlags & CLCHT_ONAVATAR && g_CluiData.bDblClkAvatars) {
				CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)contact->hContact, 0);
				return TRUE;
			}
			if (hitFlags & (CLCHT_ONITEMICON | CLCHT_ONITEMLABEL | CLCHT_ONITEMSPACE)) {
				UpdateWindow(hwnd);
				pcli->pfnDoSelectionDefaultAction(hwnd, dat);
			}
			return TRUE;
		}
		case WM_CONTEXTMENU: {
			struct ClcContact *contact;
			HMENU hMenu = NULL;
			POINT pt;
			DWORD hitFlags;

			pcli->pfnEndRename(hwnd, dat, 1);
			pcli->pfnHideInfoTip(hwnd, dat);
			KillTimer(hwnd, TIMERID_RENAME);
			KillTimer(hwnd, TIMERID_INFOTIP);
			if (GetFocus() != hwnd)
				SetFocus(hwnd);
			dat->iHotTrack = -1;
			dat->szQuickSearch[0] = 0;
			pt.x = (short) LOWORD(lParam);
			pt.y = (short) HIWORD(lParam);
			if (pt.x == -1 && pt.y == -1) {
				dat->selection = pcli->pfnGetRowByIndex(dat, dat->selection, &contact, NULL);
				if (dat->selection != -1)
					pcli->pfnEnsureVisible(hwnd, dat, dat->selection, 0);
				pt.x = dat->iconXSpace + 15;
				pt.y = RowHeights_GetItemTopY(dat, dat->selection) - dat->yScroll + (int)(dat->row_heights[dat->selection] * .7);
				hitFlags = dat->selection == -1 ? CLCHT_NOWHERE : CLCHT_ONITEMLABEL;
			} else {
				ScreenToClient(hwnd, &pt);
				dat->selection = HitTest(hwnd, dat, pt.x, pt.y, &contact, NULL, &hitFlags);
			}
			InvalidateRect(hwnd, NULL, FALSE);
			if (dat->selection != -1)
				pcli->pfnEnsureVisible(hwnd, dat, dat->selection, 0);
			UpdateWindow(hwnd);

			if (dat->selection != -1 && hitFlags & (CLCHT_ONITEMICON | CLCHT_ONITEMCHECK | CLCHT_ONITEMLABEL)) {
				if (contact->type == CLCIT_GROUP) {
					hMenu = (HMENU)CallService(MS_CLIST_MENUBUILDSUBGROUP, (WPARAM)contact->group, 0);
					ClientToScreen(hwnd, &pt);
					TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, pcli->hwndContactList, NULL);
					return 0;
					CheckMenuItem(hMenu, POPUP_GROUPHIDEOFFLINE, contact->group->hideOffline ? MF_CHECKED : MF_UNCHECKED);
				} else if (contact->type == CLCIT_CONTACT)
					hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) contact->hContact, 0);
			} else {
				//call parent for new group/hide offline menu
				PostMessage(GetParent(hwnd), WM_CONTEXTMENU, wParam, lParam);
				return 0;
			}
			if (hMenu != NULL) {
				ClientToScreen(hwnd, &pt);
				TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
				DestroyMenu(hMenu);
			}
			return 0;
		}
		case WM_COMMAND:
			if (LOWORD(wParam) == POPUP_NEWGROUP)
				SendMessage(GetParent(hwnd), msg, wParam, lParam);
			break;

		case WM_NCHITTEST: {
			LRESULT lr = SendMessage(GetParent(hwnd), WM_NCHITTEST, wParam, lParam);
			if (lr == HTLEFT || lr == HTRIGHT || lr == HTBOTTOM || lr == HTTOP || lr == HTTOPLEFT || lr == HTTOPRIGHT
					|| lr == HTBOTTOMLEFT || lr == HTBOTTOMRIGHT)
				return HTTRANSPARENT;
			break;
		}
		case WM_DESTROY: {
			int i;

			if (!dat->bisEmbedded) {
				for (i = 0; i < g_nextExtraCacheEntry; i++) {
					if (g_ExtraCache[i].floater && g_ExtraCache[i].floater->hwnd)
						DestroyWindow(g_ExtraCache[i].floater->hwnd);
				}
			}
			RowHeights_Free(dat);
			break;
		}
	}

	{
		LRESULT result = saveContactListControlWndProc(hwnd, msg, wParam, lParam);
		return result;
	}
}
