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

static TCHAR *xStatusDescr[] = { _T("Angry"), _T("Duck"), _T("Tired"), _T("Party"), _T("Beer"), _T("Thinking"), _T("Eating"), _T("TV"), _T("Friends"), _T("Coffee"),
                         _T("Music"), _T("Business"), _T("Camera"), _T("Funny"), _T("Phone"), _T("Games"), _T("College"), _T("Shopping"), _T("Sick"), _T("Sleeping"),
                         _T("Surfing"), _T("@Internet"), _T("Engeieering"), _T("Typing")};

extern struct CluiData g_CluiData;
extern struct avatarCache *g_avatarCache;
extern int g_curAvatar;
extern HANDLE hClcWindowList;
extern BOOL bAvatarThreadRunning;
extern CRITICAL_SECTION avcs;
extern HANDLE avatarUpdateQueue[];
extern DWORD dwPendingAvatarJobs;
extern HANDLE hAvatarThread;
extern HWND hwndContactTree;
extern HANDLE hExtraImageListRebuilding, hExtraImageApplying;

extern struct ExtraCache *g_ExtraCache;
extern int g_nextExtraCacheEntry, g_maxExtraCacheEntry;

//routines for managing adding/removal of items in the list, including sorting

static int AddItemToGroup(struct ClcGroup *group, int iAboveItem)
{
	if (++group->contactCount > group->allocedCount) {
		group->allocedCount += GROUP_ALLOCATE_STEP;
		group->contact = (struct ClcContact *) mir_realloc(group->contact, sizeof(struct ClcContact) * group->allocedCount);
	}
	memmove(group->contact + iAboveItem + 1, group->contact + iAboveItem, sizeof(struct ClcContact) * (group->contactCount - iAboveItem - 1));
	group->contact[iAboveItem].type = CLCIT_DIVIDER;
	group->contact[iAboveItem].flags = 0;
	memset(group->contact[iAboveItem].iExtraImage, 0xFF, sizeof(group->contact[iAboveItem].iExtraImage));
	group->contact[iAboveItem].szText[0] = '\0';
    (int)group->contact[iAboveItem].ace = (int)group->contact[iAboveItem].avatarLeft = 
        (int)group->contact[iAboveItem].extraIconRightBegin = 0;
    group->contact[iAboveItem].extraCacheEntry = -1;
	return iAboveItem;
}

struct ClcGroup *AddGroup(HWND hwnd, struct ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers)
{
	TCHAR *pBackslash, *pNextField, szThisField[sizeof(dat->list.contact[0].szText)];
	struct ClcGroup *group = &dat->list;
	int i, compareResult;

	if (!(GetWindowLong(hwnd, GWL_STYLE) & CLS_USEGROUPS))
		return &dat->list;
	pNextField = ( TCHAR* )szName;
	do {
		pBackslash = _tcschr(pNextField, '\\');
		if (pBackslash == NULL) {
			lstrcpyn(szThisField, pNextField, safe_sizeof(szThisField));
			pNextField = NULL;
		}
		else {
			lstrcpyn(szThisField, pNextField, min(safe_sizeof(szThisField), pBackslash - pNextField + 1));
			pNextField = pBackslash + 1;
		}
		compareResult = 1;
		for (i = 0; i < group->contactCount; i++) {
			if (group->contact[i].type == CLCIT_CONTACT)
				break;
			if (group->contact[i].type != CLCIT_GROUP)
				continue;
			compareResult = lstrcmp(szThisField, group->contact[i].szText);
			if (compareResult == 0) {
				if (pNextField == NULL && flags != (DWORD) - 1) {
					group->contact[i].groupId = (WORD) groupId;
					group = group->contact[i].group;
					group->expanded = (flags & GROUPF_EXPANDED) != 0;
					group->hideOffline = (flags & GROUPF_HIDEOFFLINE) != 0;
					group->groupId = groupId;
				}
				else
					group = group->contact[i].group;
				break;
			}
			if (pNextField == NULL && group->contact[i].groupId == 0)
				break;
			if (groupId && group->contact[i].groupId > groupId)
				break;
		}
		if (compareResult) {
			if (groupId == 0)
				return NULL;
			i = AddItemToGroup(group, i);
			group->contact[i].type = CLCIT_GROUP;
			lstrcpyn(group->contact[i].szText, szThisField, safe_sizeof(group->contact[i].szText));
#if defined(_UNICODE)
            RTL_DetectGroupName(&group->contact[i]);
#endif            
			group->contact[i].groupId = (WORD) (pNextField ? 0 : groupId);
			group->contact[i].group = (struct ClcGroup *) mir_alloc(sizeof(struct ClcGroup));
			group->contact[i].group->parent = group;
			group = group->contact[i].group;
			group->allocedCount = group->contactCount = 0;
			group->contact = NULL;
			if (flags == (DWORD) - 1 || pNextField != NULL) {
				group->expanded = 0;
				group->hideOffline = 0;
			}
			else {
				group->expanded = (flags & GROUPF_EXPANDED) != 0;
				group->hideOffline = (flags & GROUPF_HIDEOFFLINE) != 0;
			}
			group->groupId = pNextField ? 0 : groupId;
			group->totalMembers = 0;
			if (flags != (DWORD) - 1 && pNextField == NULL && calcTotalMembers) {
				HANDLE hContact;
				DBVARIANT dbv;
				DWORD style = GetWindowLong(hwnd, GWL_STYLE);
				hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
				while (hContact) {
					if (!DBGetContactSettingTString(hContact, "CList", "Group", &dbv)) {
						if (!lstrcmp(dbv.ptszVal, szName) && (style & CLS_SHOWHIDDEN || !CLVM_GetContactHiddenStatus(hContact, NULL, dat)))
							group->totalMembers++;
						mir_free(dbv.ptszVal);
					}
					hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
				}
			}
		}
	} while (pNextField);
	return group;
}

void FreeGroup(struct ClcGroup *group)
{
    int i;
    for (i = 0; i< group->contactCount; i++) {
        if (group->contact[i].type == CLCIT_GROUP) {
            FreeGroup(group->contact[i].group);
            mir_free(group->contact[i].group);
        }
    }
    if (group->allocedCount)
        mir_free(group->contact);
    group->allocedCount = 0;
    group->contact = NULL;
    group->contactCount = 0;
}

static int iInfoItemUniqueHandle = 0;
int AddInfoItemToGroup(struct ClcGroup *group, int flags, const TCHAR *pszText)
{
    int i = 0;

    if (flags & CLCIIF_BELOWCONTACTS)
        i = group->contactCount;
    else if (flags & CLCIIF_BELOWGROUPS) {
        for (; i< group->contactCount; i++) {
            if (group->contact[i].type == CLCIT_CONTACT)
                break;
        }
    } else
        for (; i< group->contactCount; i++) {
            if (group->contact[i].type != CLCIT_INFO)
                break;
        }
    i = AddItemToGroup(group, i);
    iInfoItemUniqueHandle = (iInfoItemUniqueHandle + 1) & 0xFFFF;
    if (iInfoItemUniqueHandle == 0)
        ++iInfoItemUniqueHandle;
    group->contact[i].type = CLCIT_INFO;
    group->contact[i].flags = (BYTE) flags;
    group->contact[i].hContact = (HANDLE)++iInfoItemUniqueHandle;
    lstrcpyn(group->contact[i].szText, pszText, safe_sizeof(group->contact[i].szText));
    group->contact[i].szText[safe_sizeof(group->contact[i].szText) - 1] = 0;
    group->contact[i].codePage = 0;
    group->contact[i].clientId = -1;
    group->contact[i].bIsMeta = 0;
    group->contact[i].xStatus = 0;
    group->contact[i].ace = NULL;
    //group->contact[i].gdipObject = NULL;
    //group->contact[i].iExtraValid = 0;
    group->contact[i].extraCacheEntry = -1;
    group->contact[i].avatarLeft = group->contact[i].extraIconRightBegin = -1;
    return i;
}

static void AddContactToGroup(struct ClcData *dat, struct ClcGroup *group, HANDLE hContact)
{
    char *szProto;
    WORD apparentMode;
    DWORD idleMode;
    DBVARIANT dbv = {0};

    int i;

    for (i = group->contactCount - 1; i >= 0; i--) {
        if (group->contact[i].type != CLCIT_INFO || !(group->contact[i].flags & CLCIIF_BELOWCONTACTS))
            break;
    }
    i = AddItemToGroup(group, i + 1);
    szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
    group->contact[i].type = CLCIT_CONTACT;
    group->contact[i].hContact = hContact;  
    group->contact[i].proto = szProto;
    group->contact[i].clientId = -1;
    group->contact[i].wStatus = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
    
    //group->contact[i].wStatus = DBGetContactSettingWord(hContact,szProto,"Status",ID_STATUS_OFFLINE);
    if (szProto != NULL && !IsHiddenMode(dat, group->contact[i].wStatus))
        group->contact[i].flags |= CONTACTF_ONLINE;
    apparentMode = szProto != NULL ? DBGetContactSettingWord(hContact, szProto, "ApparentMode", 0) : 0;
    if (apparentMode == ID_STATUS_OFFLINE)
        group->contact[i].flags |= CONTACTF_INVISTO;
    else if (apparentMode == ID_STATUS_ONLINE)
        group->contact[i].flags |= CONTACTF_VISTO;
    else if (apparentMode)
        group->contact[i].flags |= CONTACTF_VISTO | CONTACTF_INVISTO;
    if (DBGetContactSettingByte(hContact, "CList", "NotOnList", 0))
        group->contact[i].flags |= CONTACTF_NOTONLIST;
    idleMode = szProto != NULL ? DBGetContactSettingDword(hContact, szProto, "IdleTS", 0) : 0;
    if (idleMode)
        group->contact[i].flags |= CONTACTF_IDLE;
    lstrcpyn(group->contact[i].szText, GetContactDisplayNameW(hContact, 0), safe_sizeof(group->contact[i].szText));
    group->contact[i].szText[120 - MAXEXTRACOLUMNS - 1] = 0;
    group->contact[i].xStatus = DBGetContactSettingByte(hContact, szProto, "XStatusId", 0);
    if (szProto)
        group->contact[i].bIsMeta = !strcmp(group->contact[i].proto, "MetaContacts");
    else
        group->contact[i].bIsMeta = FALSE;
    if (group->contact[i].bIsMeta && g_CluiData.bMetaAvail && !(g_CluiData.dwFlags & CLUI_USEMETAICONS)) {
        group->contact[i].hSubContact = (HANDLE) CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM) hContact, 0);
        group->contact[i].metaProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) group->contact[i].hSubContact, 0);
        group->contact[i].iImage = CallService(MS_CLIST_GETCONTACTICON, (WPARAM) group->contact[i].hSubContact, 0);
    } else {
        group->contact[i].iImage = CallService(MS_CLIST_GETCONTACTICON, (WPARAM) hContact, 0);
        group->contact[i].metaProto = NULL;
    }

    if (hContact && szProto) {
        if (!DBGetContactSetting(hContact, szProto, "MirVer", &dbv)) {
            if (dbv.type == DBVT_ASCIIZ && lstrlenA(dbv.pszVal) > 1)
                GetClientID(&(group->contact[i]), dbv.pszVal);
            mir_free(dbv.pszVal);
        }
    }
    group->contact[i].codePage = DBGetContactSettingDword(hContact, "Tab_SRMsg", "ANSIcodepage", DBGetContactSettingDword(hContact, "UserInfo", "ANSIcodepage", CP_ACP));
    group->contact[i].ace = NULL;
    if(g_CluiData.bAvatarServiceAvail && (!g_CluiData.bNoOfflineAvatars || group->contact[i].wStatus != ID_STATUS_OFFLINE)) {
        group->contact[i].ace = (struct avatarCacheEntry *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)hContact, 0);
        if (group->contact[i].ace != NULL && group->contact[i].ace->cbSize != sizeof(struct avatarCacheEntry))
            group->contact[i].ace = NULL;
        if (group->contact[i].ace != NULL)
            group->contact[i].ace->t_lastAccess = time(NULL);
    }
    //group->contact[i].gdipObject = NULL;
    if(dat->bisEmbedded)
        group->contact[i].extraCacheEntry = -1;
    else {
        group->contact[i].extraCacheEntry = GetExtraCache(group->contact[i].hContact, group->contact[i].proto);
        GetExtendedInfo(&group->contact[i], dat);
        if(group->contact[i].extraCacheEntry >= 0 && group->contact[i].extraCacheEntry <= g_nextExtraCacheEntry)
            g_ExtraCache[group->contact[i].extraCacheEntry].proto_status_item = GetProtocolStatusItem(group->contact[i].bIsMeta ? group->contact[i].metaProto : group->contact[i].proto);
    }
#if defined(_UNICODE)
    RTL_DetectAndSet(&group->contact[i], group->contact[i].hContact);
#endif    
    group->contact[i].avatarLeft = group->contact[i].extraIconRightBegin = -1;
}

void AddContactToTree(HWND hwnd, struct ClcData *dat, HANDLE hContact, int updateTotalCount, int checkHideOffline)
{
    struct ClcGroup *group;
    DBVARIANT dbv;
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    WORD status;
    char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);

    if (style & CLS_NOHIDEOFFLINE)
        checkHideOffline = 0;
    if (checkHideOffline) {
        if (szProto == NULL)
            status = ID_STATUS_OFFLINE;
        else
            status = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
    }

    if (DBGetContactSettingTString(hContact, "CList", "Group", &dbv))
        group = &dat->list;
    else {
        group = AddGroup(hwnd, dat, dbv.ptszVal, (DWORD) - 1, 0, 0);
        if (group == NULL) {
            int i, len;
            DWORD groupFlags;
            TCHAR *szGroupName;
            if (!(style & CLS_HIDEEMPTYGROUPS)) {
                mir_free(dbv.ptszVal); 
                return;
            }
            if (checkHideOffline && IsHiddenMode(dat, status)) {
                for (i = 1; ; i++) {
                    szGroupName = GetGroupNameT(i, &groupFlags);
                    if (szGroupName == NULL) {
                        mir_free(dbv.ptszVal); 
                        return;   //never happens
                    }
                    if (!lstrcmp(szGroupName, dbv.ptszVal))
                        break;
                }
                if (groupFlags & GROUPF_HIDEOFFLINE) {
                    mir_free(dbv.ptszVal); 
                    return;
                }
            }
            for (i = 1; ; i++) {
                szGroupName = GetGroupNameT(i, &groupFlags);
                if (szGroupName == NULL) {
                    mir_free(dbv.ptszVal); 
                    return;
                }   //never happens
                if (!lstrcmp(szGroupName, dbv.ptszVal))
                    break;
                len = lstrlen(szGroupName);
                if (!_tcsncmp(szGroupName, dbv.ptszVal, len) && dbv.ptszVal[len] == '\\')
                    AddGroup(hwnd, dat, szGroupName, groupFlags, i, 1);
            }
            group = AddGroup(hwnd, dat, dbv.ptszVal, groupFlags, i, 1);
        }
        mir_free(dbv.ptszVal);
    }
    if (checkHideOffline) {
        if (IsHiddenMode(dat, status) && (style & CLS_HIDEOFFLINE || group->hideOffline)) {
            if (updateTotalCount)
                group->totalMembers++;
            return;
        }
    }
    AddContactToGroup(dat, group, hContact);
    if (updateTotalCount)
        group->totalMembers++;
}

struct ClcGroup * RemoveItemFromGroup(HWND hwnd, struct ClcGroup *group, struct ClcContact *contact, int updateTotalCount) {
    int iContact;

    iContact = ((unsigned) contact - (unsigned) group->contact) / sizeof(struct ClcContact);
    if (iContact >= group->contactCount)
        return group;
    if (contact->type == CLCIT_GROUP) {
        FreeGroup(contact->group);
        mir_free(contact->group);
    }
    group->contactCount--;
    if (updateTotalCount && contact->type == CLCIT_CONTACT)
        group->totalMembers--;
    memmove(group->contact + iContact, group->contact + iContact + 1, sizeof(struct ClcContact) * (group->contactCount - iContact));
    if ((GetWindowLong(hwnd, GWL_STYLE) & CLS_HIDEEMPTYGROUPS) && group->contactCount == 0) {
        int i;
        if (group->parent == NULL)
            return group;
        for (i = 0; i< group->parent->contactCount; i++) {
            if (group->parent->contact[i].groupId == group->groupId)
                break;
        }
        if (i == group->parent->contactCount)
            return group;  //never happens
        return RemoveItemFromGroup(hwnd, group->parent, &group->parent->contact[i], 0);
    }
    return group;
}

void DeleteItemFromTree(HWND hwnd, HANDLE hItem)
{
    struct ClcContact *contact;
    struct ClcGroup *group;
    struct ClcData *dat = (struct ClcData *) GetWindowLong(hwnd, 0);

    if (!FindItem(hwnd, dat, hItem, &contact, &group, NULL)) {
        DBVARIANT dbv;
        int i, nameOffset;
        if (!IsHContactContact(hItem))
            return;
        if (DBGetContactSettingTString(hItem, "CList", "Group", &dbv))
            return;

    //decrease member counts of all parent groups too
        group = &dat->list;
        nameOffset = 0;
        for (i = 0; ; i++) {
            if (group->scanIndex == group->contactCount)
                break;
            if (group->contact[i].type == CLCIT_GROUP) {
                int len = lstrlen(group->contact[i].szText);
                if (!_tcsncmp(group->contact[i].szText, dbv.ptszVal + nameOffset, len) && (dbv.ptszVal[nameOffset + len] == '\\' || dbv.ptszVal[nameOffset + len] == '\0')) {
                    group->totalMembers--;
                    if (dbv.ptszVal[nameOffset + len] == '\0')
                        break;
                }
            }
        }
        mir_free(dbv.ptszVal);
    } else
        RemoveItemFromGroup(hwnd, group, contact, 1);
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
    dat->list.contactCount = 0;
    dat->list.totalMembers = 0;
    dat->selection = -1;
    dat->SelectMode = DBGetContactSettingByte(NULL, "CLC", "SelectMode", 0); {
        int i;
        TCHAR *szGroupName;
        DWORD groupFlags;

        for (i = 1; ; i++) {
            szGroupName = GetGroupNameT(i, &groupFlags);
            if (szGroupName == NULL)
                break;
            AddGroup(hwnd, dat, szGroupName, groupFlags, i, 0);
        }
    }

    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) {
        if (style & CLS_SHOWHIDDEN || !CLVM_GetContactHiddenStatus(hContact, NULL, dat)) {
            ZeroMemory((void *)&dbv, sizeof(dbv));
            if (DBGetContactSettingTString(hContact, "CList", "Group", &dbv))
                group = &dat->list;
            else {
                group = AddGroup(hwnd, dat, dbv.ptszVal, (DWORD) - 1, 0, 0);
                mir_free(dbv.ptszVal);
            }

            if (group != NULL) {
                group->totalMembers++;
                if (!(style & CLS_NOHIDEOFFLINE) && (style & CLS_HIDEOFFLINE || group->hideOffline)) {
                    szProto = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
                    if (szProto == NULL) {
                        if (!IsHiddenMode(dat, ID_STATUS_OFFLINE))
                            AddContactToGroup(dat, group, hContact);
                    } else if (!IsHiddenMode(dat, (WORD) DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE)))
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
            if (group->scanIndex == group->contactCount) {
                group = group->parent;
                if (group == NULL)
                    break;
            } else if (group->contact[group->scanIndex].type == CLCIT_GROUP) {
                if (group->contact[group->scanIndex].group->contactCount == 0) {
                    group = RemoveItemFromGroup(hwnd, group, &group->contact[group->scanIndex], 0);
                } else {
                    group = group->contact[group->scanIndex].group;
                    group->scanIndex = 0;
                }
                continue;
            }
            group->scanIndex++;
        }
    }
    SortCLC(hwnd, dat, 0);
}

int GetGroupContentsCount(struct ClcGroup *group, int visibleOnly)
{
    int count = group->contactCount;
    struct ClcGroup *topgroup = group;

    group->scanIndex = 0;
    for (; ;) {
        if (group->scanIndex == group->contactCount) {
            if (group == topgroup)
                break;
            group = group->parent;
        } else if (group->contact[group->scanIndex].type == CLCIT_GROUP && (!visibleOnly || group->contact[group->scanIndex].group->expanded)) {
            group = group->contact[group->scanIndex].group;
            group->scanIndex = 0;
            count += group->contactCount;
            continue;
        }
        group->scanIndex++;
    }
    return count;
}

static int __cdecl GroupSortProc(const struct ClcContact *contact1, const struct ClcContact *contact2)
{
    return lstrcmpi(contact1->szText, contact2->szText);
}

static int __cdecl ContactSortProc(const struct ClcContact *contact1, const struct ClcContact *contact2)
{
    int result;

    //result=CallService(MS_CLIST_CONTACTSCOMPARE,(WPARAM)contact1->hContact,(LPARAM)contact2->hContact);
    result = InternalCompareContacts((WPARAM) contact1, (LPARAM) contact2);
    if (result)
        return result;
    //nothing to distinguish them, so make sure they stay in the same order
    return(int) contact2->hContact - (int) contact1->hContact;
}

static void InsertionSort(struct ClcContact *pContactArray, int nArray, int (*CompareProc) (const void*, const void *))
{
    int i, j;
    struct ClcContact testElement;

    for (i = 1; i < nArray; i++) {
        if (CompareProc(&pContactArray[i - 1], &pContactArray[i]) > 0) {
            testElement = pContactArray[i];
            for (j = i - 2; j >= 0; j--) {
                if (CompareProc(&pContactArray[j], &testElement) <= 0)
                    break;
            }
            j++;
            memmove(&pContactArray[j + 1], &pContactArray[j], sizeof(struct ClcContact) * (i - j));
            pContactArray[j] = testElement;
        }
    }
}

static void SortGroup(struct ClcData *dat, struct ClcGroup *group, int useInsertionSort)
{
    int i, sortCount;

    for (i = group->contactCount - 1; i >= 0; i--) {
        if (group->contact[i].type == CLCIT_DIVIDER) {
            group->contactCount--;
            memmove(&group->contact[i], &group->contact[i + 1], sizeof(struct ClcContact) * (group->contactCount - i));
        }
    }
    for (i = 0; i< group->contactCount; i++) {
        if (group->contact[i].type != CLCIT_INFO)
            break;
    }
    if (i > group->contactCount - 2)
        return;
    if (group->contact[i].type == CLCIT_GROUP) {
        if (dat->exStyle & CLS_EX_SORTGROUPSALPHA) {
            for (sortCount = 0; i + sortCount< group->contactCount; sortCount++) {
                if (group->contact[i + sortCount].type != CLCIT_GROUP)
                    break;
            }
            qsort(group->contact + i, sortCount, sizeof(struct ClcContact), GroupSortProc);
            i = i + sortCount;
        }
        for (; i< group->contactCount; i++) {
            if (group->contact[i].type == CLCIT_CONTACT)
                break;
        }
        if (group->contactCount - i < 2)
            return;
    }
    for (sortCount = 0; i + sortCount< group->contactCount; sortCount++) {
        if (group->contact[i + sortCount].type != CLCIT_CONTACT)
            break;
    }
    if (useInsertionSort)
        InsertionSort(group->contact + i, sortCount, ContactSortProc);
    else
        qsort(group->contact + i, sortCount, sizeof(struct ClcContact), ContactSortProc);
    if (dat->exStyle & CLS_EX_DIVIDERONOFF) {
        int prevContactOnline = 0;
        for (i = 0; i< group->contactCount; i++) {
            if (group->contact[i].type != CLCIT_CONTACT)
                continue;
            if (group->contact[i].flags & CONTACTF_ONLINE)
                prevContactOnline = 1;
            else {
                if (prevContactOnline) {
                    i = AddItemToGroup(group, i);
                    group->contact[i].type = CLCIT_DIVIDER;
                    lstrcpy(group->contact[i].szText, TranslateT("Offline"));
                }
                break;
            }
        }
    }
}

void __fastcall SortCLC(HWND hwnd, struct ClcData *dat, int useInsertionSort)
{
    struct ClcContact *selcontact;
    struct ClcGroup *group = &dat->list, *selgroup;
    int dividers = dat->exStyle &CLS_EX_DIVIDERONOFF;
    HANDLE hSelItem;

    if (GetRowByIndex(dat, dat->selection, &selcontact, NULL) == -1)
        hSelItem = NULL;
    else
        hSelItem = ContactToHItem(selcontact);
    group->scanIndex = 0;
    SortGroup(dat, group, useInsertionSort);
    for (; ;) {
        if (group->scanIndex == group->contactCount) {
            group = group->parent;
            if (group == NULL)
                break;
        } else if (group->contact[group->scanIndex].type == CLCIT_GROUP) {
            group = group->contact[group->scanIndex].group;
            group->scanIndex = 0;
            SortGroup(dat, group, useInsertionSort);
            continue;
        }
        group->scanIndex++;
    }
    if (hSelItem) {
        if (FindItem(hwnd, dat, hSelItem, &selcontact, &selgroup, NULL))
            dat->selection = GetRowsPriorTo(&dat->list, selgroup, selcontact - selgroup->contact);
    }
    PostMessage(hwnd, INTM_INVALIDATE, 0, 0);
    //InvalidateRect(hwnd, NULL, FALSE);
    RecalcScrollBar(hwnd, dat);
}

/*
void __fastcall __SortCLC(HWND hwnd, struct ClcData *dat, int useInsertionSort)
{
    if (g_CluiData.sortTimer >= 50 && g_CluiData.sortTimer <= 500) {
        // smaller values don't make sense, so we sort directly...
        if (!dat->bNeedSort) {
            SetTimer(hwnd, TIMERID_SORT, g_CluiData.sortTimer, NULL);
            dat->bNeedSort = TRUE;
        }
    } else {
        ReallySortCLC(hwnd, dat, useInsertionSort);
        dat->bNeedSort = FALSE;
    }
} */

struct SavedContactState_t {
    HANDLE hContact;
    BYTE iExtraImage[MAXEXTRACOLUMNS];
    BYTE iExtraValid;
    int checked;
};

struct SavedGroupState_t {
    int groupId, expanded;
};

struct SavedInfoState_t {
    int parentId;
    struct ClcContact contact;
};

void SaveStateAndRebuildList(HWND hwnd, struct ClcData *dat)
{
    NMCLISTCONTROL nm;
    int i, j;
    struct SavedGroupState_t *savedGroup = NULL;
    int savedGroupCount = 0,savedGroupAlloced = 0;
    struct SavedContactState_t *savedContact = NULL;
    int savedContactCount = 0,savedContactAlloced = 0;
    struct SavedInfoState_t *savedInfo = NULL;
    int savedInfoCount = 0,savedInfoAlloced = 0;
    struct ClcGroup *group;
    struct ClcContact *contact;

    HideInfoTip(hwnd, dat);
    KillTimer(hwnd, TIMERID_INFOTIP);
    KillTimer(hwnd, TIMERID_RENAME);
    EndRename(hwnd, dat, 1);

    group = &dat->list;
    group->scanIndex = 0;
    for (; ;) {
        if (group->scanIndex == group->contactCount) {
            group = group->parent;
            if (group == NULL)
                break;
        } else if (group->contact[group->scanIndex].type == CLCIT_GROUP) {
            group = group->contact[group->scanIndex].group;
            group->scanIndex = 0;
            if (++savedGroupCount > savedGroupAlloced) {
                savedGroupAlloced += 8;
                savedGroup = (struct SavedGroupState_t *) mir_realloc(savedGroup, sizeof(struct SavedGroupState_t) * savedGroupAlloced);
            }
            savedGroup[savedGroupCount - 1]. groupId = group->groupId;
            savedGroup[savedGroupCount - 1]. expanded = group->expanded;
            continue;
        } else if (group->contact[group->scanIndex].type == CLCIT_CONTACT) {
            if (++savedContactCount > savedContactAlloced) {
                savedContactAlloced += 16;
                savedContact = (struct SavedContactState_t *) mir_realloc(savedContact, sizeof(struct SavedContactState_t) * savedContactAlloced);
            }
            savedContact[savedContactCount - 1]. hContact = group->contact[group->scanIndex].hContact;
            //savedContact[savedContactCount - 1].iExtraValid = group->contact[group->scanIndex].iExtraValid;
            CopyMemory(savedContact[savedContactCount - 1].iExtraImage, group->contact[group->scanIndex].iExtraImage, sizeof(group->contact[group->scanIndex].iExtraImage));
            savedContact[savedContactCount - 1]. checked = group->contact[group->scanIndex].flags &CONTACTF_CHECKED;
        } else if (group->contact[group->scanIndex].type == CLCIT_INFO) {
            if (++savedInfoCount > savedInfoAlloced) {
                savedInfoAlloced += 4;
                savedInfo = (struct SavedInfoState_t *) mir_realloc(savedInfo, sizeof(struct SavedInfoState_t) * savedInfoAlloced);
            }
            if (group->parent == NULL)
                savedInfo[savedInfoCount - 1]. parentId = -1;
            else
                savedInfo[savedInfoCount - 1]. parentId = group->groupId;
            savedInfo[savedInfoCount - 1]. contact = group->contact[group->scanIndex];
        }
        group->scanIndex++;
    }

    FreeGroup(&dat->list);
    RebuildEntireList(hwnd, dat);

    group = &dat->list;
    group->scanIndex = 0;
    for (; ;) {
        if (group->scanIndex == group->contactCount) {
            group = group->parent;
            if (group == NULL)
                break;
        } else if (group->contact[group->scanIndex].type == CLCIT_GROUP) {
            group = group->contact[group->scanIndex].group;
            group->scanIndex = 0;
            for (i = 0; i < savedGroupCount; i++) {
                if (savedGroup[i].groupId == group->groupId) {
                    group->expanded = savedGroup[i].expanded;
                    break;
                }
            }
            continue;
        } else if (group->contact[group->scanIndex].type == CLCIT_CONTACT) {
            for (i = 0; i < savedContactCount; i++) {
                if (savedContact[i].hContact == group->contact[group->scanIndex].hContact) {
                    CopyMemory(group->contact[group->scanIndex].iExtraImage, savedContact[i].iExtraImage, sizeof(group->contact[group->scanIndex].iExtraImage));
                    //group->contact[group->scanIndex].iExtraValid = savedContact[i].iExtraValid;
                    if (savedContact[i].checked)
                        group->contact[group->scanIndex].flags |= CONTACTF_CHECKED;
                    break;
                }
            }
        }
        group->scanIndex++;
    }
    if (savedGroup)
        mir_free(savedGroup);
    if (savedContact)
        mir_free(savedContact);
    for (i = 0; i < savedInfoCount; i++) {
        if (savedInfo[i].parentId == -1)
            group = &dat->list;
        else {
            if (!FindItem(hwnd, dat, (HANDLE) (savedInfo[i].parentId | HCONTACT_ISGROUP), &contact, NULL, NULL))
                continue;
            group = contact->group;
        }
        j = AddInfoItemToGroup(group, savedInfo[i].contact.flags, _T(""));
        group->contact[j] = savedInfo[i].contact;
    }
    if (savedInfo)
        mir_free(savedInfo);
    RecalculateGroupCheckboxes(hwnd, dat);

    RecalcScrollBar(hwnd, dat);
    nm.hdr.code = CLN_LISTREBUILT;
    nm.hdr.hwndFrom = hwnd;
    nm.hdr.idFrom = GetDlgCtrlID(hwnd);
    SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) &nm);
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
    
    if(iExtraCacheEntry < 0 || iExtraCacheEntry > g_nextExtraCacheEntry)
        return 0;
    
    cEntry = &g_ExtraCache[iExtraCacheEntry];
    
    cEntry->bStatusMsgValid = STATUSMSG_NOTFOUND;
    hContact = cEntry->hContact;
    
    if(g_CluiData.dwFlags & CLUI_FRAME_SHOWSTATUSMSG) {
        if(!DBGetContactSettingTString(hContact, "CList", "StatusMsg", &dbv) && lstrlen(dbv.ptszVal) > 1)
            cEntry->bStatusMsgValid = STATUSMSG_CLIST;
        else {
            if(!szProto)
                szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
            if(szProto) {
                if(!DBGetContactSettingTString(hContact, szProto, "YMsg", &dbv) && lstrlen(dbv.ptszVal) > 1)
                    cEntry->bStatusMsgValid = STATUSMSG_YIM;
                else if(!DBGetContactSettingTString(hContact, szProto, "StatusDescr", &dbv) && lstrlen(dbv.ptszVal) > 1)
                    cEntry->bStatusMsgValid = STATUSMSG_GG;
                else if(!DBGetContactSettingTString(hContact, szProto, "XStatusMsg", &dbv) && lstrlen(dbv.ptszVal) > 1)
                    cEntry->bStatusMsgValid = STATUSMSG_XSTATUS;
            }
        }
    }
    if(cEntry->bStatusMsgValid == STATUSMSG_NOTFOUND) {      // no status msg, consider xstatus name (if available)
        if(!DBGetContactSettingTString(hContact, szProto, "XStatusName", &dbv) && lstrlen(dbv.ptszVal) > 1) {
            int iLen = lstrlen(dbv.ptszVal);
            cEntry->bStatusMsgValid = STATUSMSG_XSTATUSNAME;
            cEntry->statusMsg = (TCHAR *)realloc(cEntry->statusMsg, (iLen + 2) * sizeof(TCHAR));
            _tcsncpy(cEntry->statusMsg, dbv.ptszVal, iLen + 1);
            mir_free(dbv.ptszVal);
        }
        else {
            BYTE bXStatus = DBGetContactSettingByte(hContact, szProto, "XStatusId", 0);
            if(bXStatus > 0 && bXStatus <= 24) {
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
        mir_free(dbv.ptszVal);
    }
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
    if(hContact && hwndContactTree) {
        int index = GetExtraCache(hContact, NULL);
        if(index >= 0 && index < g_nextExtraCacheEntry) {
            char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
            g_ExtraCache[index].timezone = (DWORD)DBGetContactSettingByte(hContact,"UserInfo","Timezone", DBGetContactSettingByte(hContact, szProto,"Timezone",-1));
            if(g_ExtraCache[index].timezone != -1) {
                DWORD contact_gmt_diff;
                contact_gmt_diff = g_ExtraCache[index].timezone > 128 ? 256 - g_ExtraCache[index].timezone : 0 - g_ExtraCache[index].timezone;
                g_ExtraCache[index].timediff = (int)g_CluiData.local_gmt_diff - (int)contact_gmt_diff*60*60/2;
            }
            InvalidateRect(hwndContactTree, NULL, FALSE);
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
        szText = GetContactDisplayNameW(hContact, 0);
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
    
    if(group->szText && g_CluiData.bUseDCMirroring != 0) {
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

    if(index >= 0 && index < g_nextExtraCacheEntry) {
        if(g_ExtraCache[index].valid)
            return;
        g_ExtraCache[index].valid = TRUE;
    }
    else
        return;
    
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

    if(!DBGetContactSetting(contact->hContact, contact->proto, "Cellular", &dbv) && lstrlenA(dbv.pszVal) > 1)
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
    
    if(!contact->bIsMeta)
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
        if(g_nextExtraCacheEntry == g_maxExtraCacheEntry) {
            g_maxExtraCacheEntry += 100;
            g_ExtraCache = (struct ExtraCache *)realloc(g_ExtraCache, g_maxExtraCacheEntry * sizeof(struct ExtraCache));
        }
        g_ExtraCache[g_nextExtraCacheEntry].hContact = hContact;
        memset(g_ExtraCache[g_nextExtraCacheEntry].iExtraImage, 0xff, MAXEXTRACOLUMNS);
        g_ExtraCache[g_nextExtraCacheEntry].iExtraValid = 0;
        g_ExtraCache[g_nextExtraCacheEntry].valid = FALSE;
        g_ExtraCache[g_nextExtraCacheEntry].bStatusMsgValid = 0;
        g_ExtraCache[g_nextExtraCacheEntry].statusMsg = NULL;
        g_ExtraCache[g_nextExtraCacheEntry].status_item = NULL;
        LoadSkinItemToCache(&g_ExtraCache[g_nextExtraCacheEntry], szProto);
        g_ExtraCache[g_nextExtraCacheEntry].dwCFlags = g_ExtraCache[g_nextExtraCacheEntry].timediff = g_ExtraCache[g_nextExtraCacheEntry].timezone = 0;
        GetCachedStatusMsg(g_nextExtraCacheEntry, szProto);
        iFound = g_nextExtraCacheEntry++;
    }
    return iFound;
}

/*
 * checks the currently active view mode filter and returns true, if the contact should be hidden
 * if no view mode is active, it returns the CList/Hidden setting
 */

int __fastcall CLVM_GetContactHiddenStatus(HANDLE hContact, char *szProto, struct ClcData *dat)
{
    int dbHidden = DBGetContactSettingByte(hContact, "CList", "Hidden", 0);
    int filterResult = 1;
    DBVARIANT dbv = {0};
    char szTemp[64];
    TCHAR szGroupMask[256];
    DWORD dwLocalMask;
    
    // always hide subcontacts (but show them on embedded contact lists)
    
    if(g_CluiData.bMetaAvail && dat != NULL && !dat->bisEmbedded && g_CluiData.bMetaEnabled && DBGetContactSettingByte(hContact, "MetaContacts", "IsSubcontact", 0))
        return 1;

    if(g_CluiData.bFilterEffective) {
        if(szProto == NULL)
            szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
        if(g_CluiData.bFilterEffective & CLVM_STICKY_CONTACTS) {
            if((dwLocalMask = DBGetContactSettingDword(hContact, "CLVM", g_CluiData.current_viewmode, 0)) != 0) {
                if(g_CluiData.bFilterEffective & CLVM_FILTER_STICKYSTATUS) {
                    WORD wStatus = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
                    return !((1 << (wStatus - ID_STATUS_OFFLINE)) & HIWORD(dwLocalMask));
                }
                return 0;
            }
        }
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
        return (dbHidden | !filterResult);
    }
    else
        return dbHidden;
}
