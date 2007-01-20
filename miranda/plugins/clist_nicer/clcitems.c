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

TCHAR* xStatusDescr[] = 
{
	_T("Angry"), _T("Duck"), _T("Tired"), _T("Party"), _T("Beer"), _T("Thinking"), _T("Eating"), _T("TV"), _T("Friends"), _T("Coffee"),
	_T("Music"), _T("Business"), _T("Camera"), _T("Funny"), _T("Phone"), _T("Games"), _T("College"), _T("Shopping"), _T("Sick"), _T("Sleeping"),
    _T("Surfing"), _T("@Internet"), _T("Engineering"), _T("Typing"), _T("Eating..yummy.."), _T("Having fun"), _T("Chit chatting"), _T("Crashing"), _T("Going to toilet")
 };

CRITICAL_SECTION cs_extcache;
int    g_list_avatars = 0;

extern struct CluiData g_CluiData;
extern HANDLE hExtraImageListRebuilding, hExtraImageApplying;

extern struct ExtraCache *g_ExtraCache;
extern int g_nextExtraCacheEntry, g_maxExtraCacheEntry;

extern int ( *saveAddContactToGroup )(struct ClcData *dat, struct ClcGroup *group, HANDLE hContact);
extern int ( *saveAddInfoItemToGroup )(struct ClcGroup *group, int flags, const TCHAR *pszText);
extern struct ClcGroup* ( *saveRemoveItemFromGroup )(HWND hwnd, struct ClcGroup *group, struct ClcContact *contact, int updateTotalCount);

extern struct ClcGroup* ( *saveAddGroup )(HWND hwnd, struct ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers);

//routines for managing adding/removal of items in the list, including sorting

struct ClcContact* CreateClcContact( void )
{
	struct ClcContact* p = (struct ClcContact*)mir_alloc( sizeof( struct ClcContact ) );
	if ( p != NULL ) {
		ZeroMemory(p, sizeof(struct ClcContact));
		//p->clientId = -1;
		p->extraCacheEntry = -1;
		p->avatarLeft = p->extraIconRightBegin = -1;
		p->isRtl = 0;
		p->ace = 0;
	}
	return p;
}

int AddInfoItemToGroup(struct ClcGroup *group, int flags, const TCHAR *pszText)
{
	int i = saveAddInfoItemToGroup(group, flags, pszText);
	struct ClcContact* p = group->cl.items[i];
	p->codePage = 0;
	//p->clientId = -1;
	p->bIsMeta = 0;
	p->xStatus = 0;
	p->ace = NULL;
	p->extraCacheEntry = -1;
	p->avatarLeft = p->extraIconRightBegin = -1;
	return i;
}

struct ClcGroup *AddGroup(HWND hwnd, struct ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers)
{
	struct ClcGroup *p = saveAddGroup( hwnd, dat, szName, flags, groupId, calcTotalMembers);

	#if defined(_UNICODE)
		if ( p && p->parent )
			RTL_DetectGroupName( p->parent->cl.items[ p->parent->cl.count-1] );
	#else
		if ( p && p->parent ) 
			p->parent->cl.items[ p->parent->cl.count -1]->isRtl = 0;
	#endif
	return p;
}

struct ClcGroup *RemoveItemFromGroup(HWND hwnd, struct ClcGroup *group, struct ClcContact *contact, int updateTotalCount)
{
	if(contact->extraCacheEntry >= 0 && contact->extraCacheEntry < g_nextExtraCacheEntry) {
		if(g_ExtraCache[contact->extraCacheEntry].floater && g_ExtraCache[contact->extraCacheEntry].floater->hwnd)
			ShowWindow(g_ExtraCache[contact->extraCacheEntry].floater->hwnd, SW_HIDE);
	}
	return(saveRemoveItemFromGroup(hwnd, group, contact, updateTotalCount));
}

void LoadAvatarForContact(struct ClcContact *p)
{
    DWORD dwFlags;

    if(p->extraCacheEntry >= 0 && p->extraCacheEntry <= g_maxExtraCacheEntry)
        dwFlags = g_ExtraCache[p->extraCacheEntry].dwDFlags;
    else
        dwFlags = DBGetContactSettingDword(p->hContact, "CList", "CLN_Flags", 0);

    if(g_CluiData.dwFlags & CLUI_FRAME_AVATARS)
        p->cFlags = (dwFlags & ECF_HIDEAVATAR ? p->cFlags & ~ECF_AVATAR : p->cFlags | ECF_AVATAR);
    else
        p->cFlags = (dwFlags & ECF_FORCEAVATAR ? p->cFlags | ECF_AVATAR : p->cFlags & ~ECF_AVATAR);

    p->ace = NULL;
    if(g_CluiData.bAvatarServiceAvail && (p->cFlags & ECF_AVATAR) && (!g_CluiData.bNoOfflineAvatars || p->wStatus != ID_STATUS_OFFLINE)) {
        p->ace = (struct avatarCacheEntry *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)p->hContact, 0);
        if (p->ace != NULL && p->ace->cbSize != sizeof(struct avatarCacheEntry))
            p->ace = NULL;
        if (p->ace != NULL)
            p->ace->t_lastAccess = g_CluiData.t_now;
    }
    if(p->ace == NULL)
        p->cFlags &= ~ECF_AVATAR;
}

int AddContactToGroup(struct ClcData *dat, struct ClcGroup *group, HANDLE hContact)
{
	int i = saveAddContactToGroup( dat, group, hContact );
	struct ClcContact* p = group->cl.items[i];

	p->wStatus = DBGetContactSettingWord(hContact, p->proto, "Status", ID_STATUS_OFFLINE);
	p->xStatus = DBGetContactSettingByte(hContact, p->proto, "XStatusId", 0);
	if (p->proto)
		p->bIsMeta = !strcmp(p->proto, "MetaContacts");
	else
		p->bIsMeta = FALSE;
	if (p->bIsMeta && g_CluiData.bMetaAvail && !(g_CluiData.dwFlags & CLUI_USEMETAICONS)) {
		p->hSubContact = (HANDLE) CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM) hContact, 0);
		p->metaProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) p->hSubContact, 0);
		p->iImage = CallService(MS_CLIST_GETCONTACTICON, (WPARAM) p->hSubContact, 0);
	} else {
		p->iImage = CallService(MS_CLIST_GETCONTACTICON, (WPARAM) hContact, 0);
		p->metaProto = NULL;
	}

	p->codePage = DBGetContactSettingDword(hContact, "Tab_SRMsg", "ANSIcodepage", DBGetContactSettingDword(hContact, "UserInfo", "ANSIcodepage", CP_ACP));
    p->bSecondLine = DBGetContactSettingByte(hContact, "CList", "CLN_2ndline", g_CluiData.dualRowMode);

	if(dat->bisEmbedded)
		p->extraCacheEntry = -1;
	else {
		p->extraCacheEntry = GetExtraCache(p->hContact, p->proto);
		GetExtendedInfo( p, dat);
		if(p->extraCacheEntry >= 0 && p->extraCacheEntry < g_nextExtraCacheEntry) {
			g_ExtraCache[p->extraCacheEntry].proto_status_item = GetProtocolStatusItem(p->bIsMeta ? p->metaProto : p->proto);
			if(DBGetContactSettingByte(p->hContact, "CList", "floating", 0)) {
				if(g_ExtraCache[p->extraCacheEntry].floater == NULL)
					FLT_Create(p->extraCacheEntry);
				else {
					ShowWindow(g_ExtraCache[p->extraCacheEntry].floater->hwnd, SW_SHOWNOACTIVATE);
					FLT_Update(dat, p);
				}
			}
		}
	}
    LoadAvatarForContact(p);
#if defined(_UNICODE)
	RTL_DetectAndSet( p, p->hContact);
#endif    
	p->avatarLeft = p->extraIconRightBegin = -1;
	p->flags |= DBGetContactSettingByte(p->hContact, "CList", "Priority", 0) ? CONTACTF_PRIORITY : 0;

	return i;
}

void RebuildEntireList(HWND hwnd, struct ClcData *dat)
{
	char *szProto;
	DWORD style = GetWindowLong(hwnd, GWL_STYLE);
	HANDLE hContact;
	struct ClcGroup *group;
	DBVARIANT dbv = {0};

	RowHeights_Clear(dat);
	RowHeights_GetMaxRowHeight(dat, hwnd);

	dat->list.expanded = 1;
	dat->list.hideOffline = DBGetContactSettingByte(NULL, "CLC", "HideOfflineRoot", 0);
	dat->list.cl.count = 0;
	dat->list.totalMembers = 0;
	dat->selection = -1;
	dat->SelectMode = DBGetContactSettingByte(NULL, "CLC", "SelectMode", 0); {
		int i;
		TCHAR *szGroupName;
		DWORD groupFlags;

		for (i = 1; ; i++) {
			szGroupName = pcli->pfnGetGroupName(i, &groupFlags);
			if (szGroupName == NULL)
				break;
			pcli->pfnAddGroup(hwnd, dat, szGroupName, groupFlags, i, 0);
		}
	}

	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) {
		if (style & CLS_SHOWHIDDEN || !CLVM_GetContactHiddenStatus(hContact, NULL, dat)) {
			ZeroMemory((void *)&dbv, sizeof(dbv));
			if (DBGetContactSettingTString(hContact, "CList", "Group", &dbv))
				group = &dat->list;
			else {
				group = pcli->pfnAddGroup(hwnd, dat, dbv.ptszVal, (DWORD) - 1, 0, 0);
				mir_free(dbv.ptszVal);
			}

			if (group != NULL) {
				group->totalMembers++;
				if (!(style & CLS_NOHIDEOFFLINE) && (style & CLS_HIDEOFFLINE || group->hideOffline)) {
					szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
					if (szProto == NULL) {
						if (!pcli->pfnIsHiddenMode(dat, ID_STATUS_OFFLINE))
							AddContactToGroup(dat, group, hContact);
					} else if (!pcli->pfnIsHiddenMode(dat, (WORD) DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE)))
						AddContactToGroup(dat, group, hContact);
				} else
					AddContactToGroup(dat, group, hContact);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}

	if (style & CLS_HIDEEMPTYGROUPS) {
		group = &dat->list;
		group->scanIndex = 0;
		for (; ;) {
			if (group->scanIndex == group->cl.count) {
				group = group->parent;
				if (group == NULL)
					break;
			} else if (group->cl.items[group->scanIndex]->type == CLCIT_GROUP) {
				if (group->cl.items[group->scanIndex]->group->cl.count == 0) {
					group = pcli->pfnRemoveItemFromGroup(hwnd, group, group->cl.items[group->scanIndex], 0);
				} else {
					group = group->cl.items[group->scanIndex]->group;
					group->scanIndex = 0;
				}
				continue;
			}
			group->scanIndex++;
		}
	}
	pcli->pfnSortCLC(hwnd, dat, 0);
	if(!dat->bisEmbedded)
		FLT_SyncWithClist();
}

/*
 * status msg in the database has changed.
 * get it and store it properly formatted in the extra data cache
 */

BYTE GetCachedStatusMsg(int iExtraCacheEntry, char *szProto)
{
	DBVARIANT dbv = {0};
	HANDLE hContact;
	struct ExtraCache *cEntry;
	int result;

	if(iExtraCacheEntry < 0 || iExtraCacheEntry > g_nextExtraCacheEntry)
		return 0;

	cEntry = &g_ExtraCache[iExtraCacheEntry];

	cEntry->bStatusMsgValid = STATUSMSG_NOTFOUND;
	hContact = cEntry->hContact;

	result = DBGetContactSettingTString(hContact, "CList", "StatusMsg", &dbv);
	if ( !result && lstrlen(dbv.ptszVal) > 1)
		cEntry->bStatusMsgValid = STATUSMSG_CLIST;
	else {
		if(!szProto)
			szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if(szProto) {
			if ( !result )
				DBFreeVariant( &dbv );
			if( !( result = DBGetContactSettingTString(hContact, szProto, "YMsg", &dbv)) && lstrlen(dbv.ptszVal) > 1)
				cEntry->bStatusMsgValid = STATUSMSG_YIM;
			else if ( !(result = DBGetContactSettingTString(hContact, szProto, "StatusDescr", &dbv)) && lstrlen(dbv.ptszVal) > 1)
				cEntry->bStatusMsgValid = STATUSMSG_GG;
			else if( !(result = DBGetContactSettingTString(hContact, szProto, "XStatusMsg", &dbv)) && lstrlen(dbv.ptszVal) > 1)
				cEntry->bStatusMsgValid = STATUSMSG_XSTATUS;
	}	}

	if(cEntry->bStatusMsgValid == STATUSMSG_NOTFOUND) {      // no status msg, consider xstatus name (if available)
		result = DBGetContactSettingTString(hContact, szProto, "XStatusName", &dbv);
		if ( !result && lstrlen(dbv.ptszVal) > 1) {
			int iLen = lstrlen(dbv.ptszVal);
			cEntry->bStatusMsgValid = STATUSMSG_XSTATUSNAME;
			cEntry->statusMsg = (TCHAR *)realloc(cEntry->statusMsg, (iLen + 2) * sizeof(TCHAR));
			_tcsncpy(cEntry->statusMsg, dbv.ptszVal, iLen + 1);
		}
		else {
			BYTE bXStatus = DBGetContactSettingByte(hContact, szProto, "XStatusId", 0);
			if(bXStatus > 0 && bXStatus <= 29) {
				TCHAR *szwXstatusName = TranslateTS(xStatusDescr[bXStatus - 1]);
				cEntry->statusMsg = (TCHAR *)realloc(cEntry->statusMsg, (lstrlen(szwXstatusName) + 2) * sizeof(TCHAR));
				_tcsncpy(cEntry->statusMsg, szwXstatusName, lstrlen(szwXstatusName) + 1);
				cEntry->bStatusMsgValid = STATUSMSG_XSTATUSNAME;
			}
		}
	}
	if(cEntry->bStatusMsgValid > STATUSMSG_XSTATUSNAME) {
		int j = 0, i;
		cEntry->statusMsg = (TCHAR *)realloc(cEntry->statusMsg, (lstrlen(dbv.ptszVal) + 2) * sizeof(TCHAR));
		for(i = 0; dbv.ptszVal[i]; i++) {
			if(dbv.ptszVal[i] == (TCHAR)0x0d)
				continue;
			cEntry->statusMsg[j] = dbv.ptszVal[i] == (wchar_t)0x0a ? (wchar_t)' ' : dbv.ptszVal[i];
			j++;
		}
		cEntry->statusMsg[j] = (TCHAR)0;
	}
	if ( !result )
		DBFreeVariant( &dbv );

#if defined(_UNICODE)
	if(cEntry->bStatusMsgValid != STATUSMSG_NOTFOUND) {
		WORD infoTypeC2[12];
		int iLen, i
			;
		ZeroMemory(infoTypeC2, sizeof(WORD) * 12);
		iLen = min(lstrlenW(cEntry->statusMsg), 10);
		GetStringTypeW(CT_CTYPE2, cEntry->statusMsg, iLen, infoTypeC2);
		cEntry->dwCFlags &= ~ECF_RTLSTATUSMSG;
		for(i = 0; i < 10; i++) {
			if(infoTypeC2[i] == C2_RIGHTTOLEFT) {
				cEntry->dwCFlags |= ECF_RTLSTATUSMSG;
				break;
			}
		}
	}
#endif    
	return cEntry->bStatusMsgValid;;
}

void ReloadExtraInfo(HANDLE hContact)
{
	if(hContact && pcli->hwndContactTree) {
		int index = GetExtraCache(hContact, NULL);
		if(index >= 0 && index < g_nextExtraCacheEntry) {
			char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
			g_ExtraCache[index].timezone = (DWORD)DBGetContactSettingByte(hContact,"UserInfo","Timezone", DBGetContactSettingByte(hContact, szProto,"Timezone",-1));
			if(g_ExtraCache[index].timezone != -1) {
				DWORD contact_gmt_diff;
				contact_gmt_diff = g_ExtraCache[index].timezone > 128 ? 256 - g_ExtraCache[index].timezone : 0 - g_ExtraCache[index].timezone;
				g_ExtraCache[index].timediff = (int)g_CluiData.local_gmt_diff - (int)contact_gmt_diff*60*60/2;
			}
			InvalidateRect(pcli->hwndContactTree, NULL, FALSE);
		}
	}
}

/*
 * autodetect RTL property of the nickname, evaluates the first 10 characters of the nickname only
 */

#if defined(_UNICODE)
void RTL_DetectAndSet(struct ClcContact *contact, HANDLE hContact)
{
    WORD infoTypeC2[12];
    int i, index;
    TCHAR *szText = NULL;
    DWORD iLen;
    
    ZeroMemory(infoTypeC2, sizeof(WORD) * 12);
    
    if(contact == NULL) {
        szText = pcli->pfnGetContactDisplayName(hContact, 0);
        index = GetExtraCache(hContact, NULL);
    }
    else {
        szText = contact->szText;
        index = contact->extraCacheEntry;
    }
    if(index >= 0 && index < g_nextExtraCacheEntry) {
        iLen = min(lstrlenW(szText), 10);
        GetStringTypeW(CT_CTYPE2, szText, iLen, infoTypeC2);
        g_ExtraCache[index].dwCFlags &= ~ECF_RTLNICK;
        for(i = 0; i < 10; i++) {
            if(infoTypeC2[i] == C2_RIGHTTOLEFT) {
                g_ExtraCache[index].dwCFlags |= ECF_RTLNICK;
                return;
            }
        }
    }
}

void RTL_DetectGroupName(struct ClcContact *group)
{
    WORD infoTypeC2[12];
    int i;
    DWORD iLen;

    group->isRtl = 0;
    
    if(group->szText) {
        iLen = min(lstrlenW(group->szText), 10);
        GetStringTypeW(CT_CTYPE2, group->szText, iLen, infoTypeC2);
        for(i = 0; i < 10; i++) {
            if(infoTypeC2[i] == C2_RIGHTTOLEFT) {
                group->isRtl = 1;
                return;
            }
        }
    }
}
#endif
/*
 * check for exteneded user information - email, phone numbers, homepage
 * set extra icons accordingly
 */

void GetExtendedInfo(struct ClcContact *contact, struct ClcData *dat)
{
    DBVARIANT dbv = {0};
    BOOL iCacheNew = FALSE;
    int index;

    if(dat->bisEmbedded || contact == NULL)
        return;
    
    if(contact->proto == NULL || contact->hContact == 0)
        return;
    
    index = contact->extraCacheEntry;

    //firstTime = DBGetContactSettingDword(contact->hContact, "CList", "mf_firstEvent", 0);
    //count = DBGetContactSettingDword(contact->hContact, "CList", "mf_count", 0);
    //new_freq = count ? (g_CluiData.t_now - firstTime) / count : 0x7fffffff;
    g_ExtraCache[index].msgFrequency = DBGetContactSettingDword(contact->hContact, "CList", "mf_freq", 0x7fffffff);
    //g_ExtraCache[index].msgFrequency = new_freq;
    //DBWriteContactSettingDword(contact->hContact, "CList", "mf_freq", new_freq);

    if(index >= 0 && index < g_nextExtraCacheEntry) {
        if(g_ExtraCache[index].valid)
            return;
        g_ExtraCache[index].valid = TRUE;
    }
    else
        return;
    
    g_ExtraCache[index].isChatRoom = DBGetContactSettingByte(contact->hContact, contact->proto, "ChatRoom", 0);

    g_ExtraCache[index].iExtraValid &= ~(EIMG_SHOW_MAIL | EIMG_SHOW_SMS | EIMG_SHOW_URL);
    g_ExtraCache[index].iExtraImage[EIMG_MAIL] = g_ExtraCache[index].iExtraImage[EIMG_URL] = g_ExtraCache[index].iExtraImage[EIMG_SMS] = 0xff;

    if(!DBGetContactSetting(contact->hContact, contact->proto, "e-mail", &dbv) && lstrlenA(dbv.pszVal) > 1)
        g_ExtraCache[index].iExtraImage[EIMG_MAIL] = 0;
    else if(!DBGetContactSetting(contact->hContact, "UserInfo", "Mye-mail0", &dbv) && lstrlenA(dbv.pszVal) > 1)
        g_ExtraCache[index].iExtraImage[EIMG_MAIL] = 0;

    if(dbv.pszVal) {
        mir_free(dbv.pszVal);
        dbv.pszVal = NULL;
    }

    if(!DBGetContactSetting(contact->hContact, contact->proto, "Homepage", &dbv) && lstrlenA(dbv.pszVal) > 1)
        g_ExtraCache[index].iExtraImage[EIMG_URL] = 1;
    else if(!DBGetContactSetting(contact->hContact, "UserInfo", "Homepage", &dbv) && lstrlenA(dbv.pszVal) > 1)
        g_ExtraCache[index].iExtraImage[EIMG_URL] = 1;
    
    if(dbv.pszVal) {
        mir_free(dbv.pszVal);
        dbv.pszVal = NULL;
    }

    if(!DBGetContactSetting(contact->hContact, "UserInfoEx", "Cellular", &dbv) && lstrlenA(dbv.pszVal) > 1)
        g_ExtraCache[index].iExtraImage[EIMG_SMS] = 2;
    else if(!DBGetContactSetting(contact->hContact, contact->proto, "Cellular", &dbv) && lstrlenA(dbv.pszVal) > 1)
        g_ExtraCache[index].iExtraImage[EIMG_SMS] = 2;
    else if(!DBGetContactSetting(contact->hContact, "UserInfo", "MyPhone0", &dbv) && lstrlenA(dbv.pszVal) > 1)
        g_ExtraCache[index].iExtraImage[EIMG_SMS] = 2;
    
    if(dbv.pszVal) {
        mir_free(dbv.pszVal);
        dbv.pszVal = NULL;
    }

    // set the mask for valid extra images...
    
    g_ExtraCache[index].iExtraValid |= ((g_ExtraCache[index].iExtraImage[EIMG_MAIL] != 0xff ? EIMG_SHOW_MAIL : 0) | 
        (g_ExtraCache[index].iExtraImage[EIMG_URL] != 0xff ? EIMG_SHOW_URL : 0) |
        (g_ExtraCache[index].iExtraImage[EIMG_SMS] != 0xff ? EIMG_SHOW_SMS : 0));


    g_ExtraCache[index].timezone = (DWORD)DBGetContactSettingByte(contact->hContact,"UserInfo","Timezone", DBGetContactSettingByte(contact->hContact, contact->proto,"Timezone",-1));
    if(g_ExtraCache[index].timezone != -1) {
        DWORD contact_gmt_diff;
        contact_gmt_diff = g_ExtraCache[index].timezone > 128 ? 256 - g_ExtraCache[index].timezone : 0 - g_ExtraCache[index].timezone;
        g_ExtraCache[index].timediff = (int)g_CluiData.local_gmt_diff - (int)contact_gmt_diff*60*60/2;
    }

    // notify other plugins to re-supply their extra images (icq for xstatus, mBirthday etc...)
    
    NotifyEventHooks(hExtraImageApplying, (WPARAM)contact->hContact, 0);
}

static void LoadSkinItemToCache(struct ExtraCache *cEntry, char *szProto)
{
    HANDLE hContact = cEntry->hContact;
    
    if(DBGetContactSettingByte(hContact, "EXTBK", "VALID", 0)) {
        if(cEntry->status_item == NULL)
            cEntry->status_item = malloc(sizeof(StatusItems_t));
        ZeroMemory(cEntry->status_item, sizeof(StatusItems_t));
        strcpy(cEntry->status_item->szName, "{--CONTACT--}");           // mark as "per contact" item
        cEntry->status_item->IGNORED = 0;

        cEntry->status_item->TEXTCOLOR = DBGetContactSettingDword(hContact, "EXTBK", "TEXT", RGB(20, 20, 20));
        cEntry->status_item->COLOR = DBGetContactSettingDword(hContact, "EXTBK", "COLOR1", RGB(224, 224, 224));
        cEntry->status_item->COLOR2 = DBGetContactSettingDword(hContact, "EXTBK", "COLOR2", RGB(224, 224, 224));
        cEntry->status_item->ALPHA = (BYTE)DBGetContactSettingByte(hContact, "EXTBK", "ALPHA", 100);

        cEntry->status_item->MARGIN_LEFT = (DWORD)DBGetContactSettingByte(hContact, "EXTBK", "LEFT", 0);
        cEntry->status_item->MARGIN_RIGHT = (DWORD)DBGetContactSettingByte(hContact, "EXTBK", "RIGHT", 0);
        cEntry->status_item->MARGIN_TOP = (DWORD)DBGetContactSettingByte(hContact, "EXTBK", "TOP", 0);
        cEntry->status_item->MARGIN_BOTTOM = (DWORD)DBGetContactSettingByte(hContact, "EXTBK", "BOTTOM", 0);

        cEntry->status_item->COLOR2_TRANSPARENT = (BYTE)DBGetContactSettingByte(hContact, "EXTBK", "TRANS", 1);
        cEntry->status_item->BORDERSTYLE = DBGetContactSettingDword(hContact, "EXTBK", "BDR", 0);

        cEntry->status_item->CORNER = DBGetContactSettingByte(hContact, "EXTBK", "CORNER", 0);
        cEntry->status_item->GRADIENT = DBGetContactSettingByte(hContact, "EXTBK", "GRAD", 0);
    }
    else if(cEntry->status_item) {
        free(cEntry->status_item);
        cEntry->status_item = NULL;
    }
}

void ReloadSkinItemsToCache()
{
    int i;
    char *szProto;
    
    for(i = 0; i < g_nextExtraCacheEntry; i++) {
        szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)g_ExtraCache[i].hContact, 0);
        if(szProto)
            LoadSkinItemToCache(&g_ExtraCache[i], szProto);
    }
}

int GetExtraCache(HANDLE hContact, char *szProto)
{
    int i, iFound = -1;

    for(i = 0; i < g_nextExtraCacheEntry; i++) {
        if(g_ExtraCache[i].hContact == hContact) {
            iFound = i;
            break;
        }
    }
    if(iFound == -1) {
		EnterCriticalSection(&cs_extcache);
        if(g_nextExtraCacheEntry == g_maxExtraCacheEntry) {
            g_maxExtraCacheEntry += 100;
            g_ExtraCache = (struct ExtraCache *)realloc(g_ExtraCache, g_maxExtraCacheEntry * sizeof(struct ExtraCache));
        }
        memset(&g_ExtraCache[g_nextExtraCacheEntry], 0, sizeof(struct ExtraCache));
		g_ExtraCache[g_nextExtraCacheEntry].hContact = hContact;
        memset(g_ExtraCache[g_nextExtraCacheEntry].iExtraImage, 0xff, MAXEXTRACOLUMNS);
        g_ExtraCache[g_nextExtraCacheEntry].iExtraValid = 0;
        g_ExtraCache[g_nextExtraCacheEntry].valid = FALSE;
        g_ExtraCache[g_nextExtraCacheEntry].bStatusMsgValid = 0;
        g_ExtraCache[g_nextExtraCacheEntry].statusMsg = NULL;
        g_ExtraCache[g_nextExtraCacheEntry].status_item = NULL;
        LoadSkinItemToCache(&g_ExtraCache[g_nextExtraCacheEntry], szProto);
        g_ExtraCache[g_nextExtraCacheEntry].dwCFlags = g_ExtraCache[g_nextExtraCacheEntry].timediff = g_ExtraCache[g_nextExtraCacheEntry].timezone = 0;
        g_ExtraCache[g_nextExtraCacheEntry].dwDFlags = DBGetContactSettingDword(hContact, "CList", "CLN_Flags", 0);
        GetCachedStatusMsg(g_nextExtraCacheEntry, szProto);
		g_ExtraCache[g_nextExtraCacheEntry].dwLastMsgTime = INTSORT_GetLastMsgTime(hContact);
        iFound = g_nextExtraCacheEntry++;
		LeaveCriticalSection(&cs_extcache);
    }
    return iFound;
}

/*
 * checks the currently active view mode filter and returns true, if the contact should be hidden
 * if no view mode is active, it returns the CList/Hidden setting
 * also cares about sub contacts (if meta is active)
 */

int __fastcall CLVM_GetContactHiddenStatus(HANDLE hContact, char *szProto, struct ClcData *dat)
{
    int dbHidden = DBGetContactSettingByte(hContact, "CList", "Hidden", 0);		// default hidden state, always respect it.
    int filterResult = 1;
    DBVARIANT dbv = {0};
    char szTemp[64];
    TCHAR szGroupMask[256];
    DWORD dwLocalMask;
    
    // always hide subcontacts (but show them on embedded contact lists)
    
    if(g_CluiData.bMetaAvail && dat != NULL && dat->bHideSubcontacts && g_CluiData.bMetaEnabled && DBGetContactSettingByte(hContact, "MetaContacts", "IsSubcontact", 0))
        return 1;

    if(g_CluiData.bFilterEffective) {
        if(szProto == NULL)
            szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		// check stickies first (priority), only if we really have stickies defined (CLVM_STICKY_CONTACTS is set).
        if(g_CluiData.bFilterEffective & CLVM_STICKY_CONTACTS) {
            if((dwLocalMask = DBGetContactSettingDword(hContact, "CLVM", g_CluiData.current_viewmode, 0)) != 0) {
                if(g_CluiData.bFilterEffective & CLVM_FILTER_STICKYSTATUS) {
                    WORD wStatus = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
                    return !((1 << (wStatus - ID_STATUS_OFFLINE)) & HIWORD(dwLocalMask));
                }
                return 0;
            }
        }
        // check the proto, use it as a base filter result for all further checks
		if(g_CluiData.bFilterEffective & CLVM_FILTER_PROTOS) {
            mir_snprintf(szTemp, sizeof(szTemp), "%s|", szProto);
            filterResult = strstr(g_CluiData.protoFilter, szTemp) ? 1 : 0;
        }
        if(g_CluiData.bFilterEffective & CLVM_FILTER_GROUPS) {
            if(!DBGetContactSettingTString(hContact, "CList", "Group", &dbv)) {
                _sntprintf(szGroupMask, safe_sizeof(szGroupMask), _T("%s|"), &dbv.ptszVal[1]);
                filterResult = (g_CluiData.filterFlags & CLVM_PROTOGROUP_OP) ? (filterResult | (_tcsstr(g_CluiData.groupFilter, szGroupMask) ? 1 : 0)) : (filterResult & (_tcsstr(g_CluiData.groupFilter, szGroupMask) ? 1 : 0));
                mir_free(dbv.ptszVal);
            }
            else if(g_CluiData.filterFlags & CLVM_INCLUDED_UNGROUPED)
                filterResult = (g_CluiData.filterFlags & CLVM_PROTOGROUP_OP) ? filterResult : filterResult & 1;
            else
                filterResult = (g_CluiData.filterFlags & CLVM_PROTOGROUP_OP) ? filterResult : filterResult & 0;
        }
        if(g_CluiData.bFilterEffective & CLVM_FILTER_STATUS) {
            WORD wStatus = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
            filterResult = (g_CluiData.filterFlags & CLVM_GROUPSTATUS_OP) ? ((filterResult | ((1 << (wStatus - ID_STATUS_OFFLINE)) & g_CluiData.statusMaskFilter ? 1 : 0))) : (filterResult & ((1 << (wStatus - ID_STATUS_OFFLINE)) & g_CluiData.statusMaskFilter ? 1 : 0));
        }
		if(g_CluiData.bFilterEffective & CLVM_FILTER_LASTMSG) {
			DWORD now;
			int iEntry = GetExtraCache(hContact, szProto);
			if(iEntry >= 0 && iEntry <= g_nextExtraCacheEntry) {
				now = g_CluiData.t_now;
				now -= g_CluiData.lastMsgFilter;
				if(g_CluiData.bFilterEffective & CLVM_FILTER_LASTMSG_OLDERTHAN)
					filterResult = filterResult & (g_ExtraCache[iEntry].dwLastMsgTime < now);
				else if(g_CluiData.bFilterEffective & CLVM_FILTER_LASTMSG_NEWERTHAN)
					filterResult = filterResult & (g_ExtraCache[iEntry].dwLastMsgTime > now);
			}
		}
        return (dbHidden | !filterResult);
    }
    else
        return dbHidden;
}
