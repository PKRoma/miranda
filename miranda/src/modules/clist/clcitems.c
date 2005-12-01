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
#include "clc.h"
#include "../database/dblists.h"

//routines for managing adding/removal of items in the list, including sorting

int fnAddItemToGroup(struct ClcGroup *group, int iAboveItem)
{
	struct ClcContact* newItem = cli.pfnCreateClcContact();
	newItem->type = CLCIT_DIVIDER;
	newItem->flags = 0;
	newItem->szText[0] = '\0';
	memset( newItem->iExtraImage, 0xFF, SIZEOF(newItem->iExtraImage));

	List_Insert(( SortedList* )&group->cl, newItem, iAboveItem );
	return iAboveItem;
}

struct ClcGroup* fnAddGroup(HWND hwnd, struct ClcData *dat, const TCHAR *szName, DWORD flags, int groupId, int calcTotalMembers)
{
	TCHAR *pBackslash, *pNextField, szThisField[ SIZEOF(dat->list.cl.items[0]->szText) ];
	struct ClcGroup *group = &dat->list;
	int i, compareResult;

	dat->needsResort = 1;
	if (!(GetWindowLong(hwnd, GWL_STYLE) & CLS_USEGROUPS))
		return &dat->list;

	pNextField = ( TCHAR* )szName;
	do {
		pBackslash = _tcschr(pNextField, '\\');
		if (pBackslash == NULL) {
			lstrcpyn(szThisField, pNextField, SIZEOF(szThisField));
			pNextField = NULL;
		}
		else {
			lstrcpyn(szThisField, pNextField, min( SIZEOF(szThisField), pBackslash - pNextField + 1 ));
			pNextField = pBackslash + 1;
		}
		compareResult = 1;
		for (i = 0; i < group->cl.count; i++) {
			if (group->cl.items[i]->type == CLCIT_CONTACT)
				break;
			if (group->cl.items[i]->type != CLCIT_GROUP)
				continue;
			compareResult = lstrcmp(szThisField, group->cl.items[i]->szText);
			if (compareResult == 0) {
				if (pNextField == NULL && flags != (DWORD) - 1) {
					group->cl.items[i]->groupId = (WORD) groupId;
					group = group->cl.items[i]->group;
					group->expanded = (flags & GROUPF_EXPANDED) != 0;
					group->hideOffline = (flags & GROUPF_HIDEOFFLINE) != 0;
					group->groupId = groupId;
				}
				else
					group = group->cl.items[i]->group;
				break;
			}
			if (pNextField == NULL && group->cl.items[i]->groupId == 0)
				break;
			if (groupId && group->cl.items[i]->groupId > groupId)
				break;
		}
		if (compareResult) {
			if (groupId == 0)
				return NULL;
			i = cli.pfnAddItemToGroup(group, i);
			group->cl.items[i]->type = CLCIT_GROUP;
			lstrcpyn(group->cl.items[i]->szText, szThisField, SIZEOF( group->cl.items[i]->szText ));
			group->cl.items[i]->groupId = (WORD) (pNextField ? 0 : groupId);
			group->cl.items[i]->group = (struct ClcGroup *) malloc(sizeof(struct ClcGroup));
			group->cl.items[i]->group->parent = group;
			group = group->cl.items[i]->group;
			memset( &group->cl, 0, sizeof( group->cl ));
         group->cl.increment = 10;
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
				DWORD style = GetWindowLong(hwnd, GWL_STYLE);
				HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
				while (hContact) {
					ClcCacheEntryBase* cache = cli.pfnGetCacheEntry( hContact );
					if ( !lstrcmp( cache->group, szName) && (style & CLS_SHOWHIDDEN || !cache->isHidden ))
						group->totalMembers++;

					hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
				}
			}
		}
	} while (pNextField);
	return group;
}

void fnFreeGroup(struct ClcGroup *group)
{
	int i;
	for (i = 0; i < group->cl.count; i++) {
		if (group->cl.items[i]->type == CLCIT_GROUP) {
			cli.pfnFreeGroup(group->cl.items[i]->group);
			free(group->cl.items[i]->group);
		}
		free(group->cl.items[i]);
	}
	if (group->cl.count)
		free(group->cl.items);
	group->cl.limit = group->cl.count = 0;
	group->cl.items = NULL;
}

static int iInfoItemUniqueHandle = 0;
int fnAddInfoItemToGroup(struct ClcGroup *group, int flags, const TCHAR *pszText)
{
	int i = 0;

	if (flags & CLCIIF_BELOWCONTACTS)
		i = group->cl.count;
	else if (flags & CLCIIF_BELOWGROUPS) {
		for (; i < group->cl.count; i++)
			if (group->cl.items[i]->type == CLCIT_CONTACT)
				break;
	}
	else
		for (; i < group->cl.count; i++)
			if (group->cl.items[i]->type != CLCIT_INFO)
				break;
	i = cli.pfnAddItemToGroup(group, i);
	iInfoItemUniqueHandle = (iInfoItemUniqueHandle + 1) & 0xFFFF;
	if (iInfoItemUniqueHandle == 0)
		++iInfoItemUniqueHandle;
	group->cl.items[i]->type = CLCIT_INFO;
	group->cl.items[i]->flags = (BYTE) flags;
	group->cl.items[i]->hContact = (HANDLE)++ iInfoItemUniqueHandle;
	lstrcpyn(group->cl.items[i]->szText, pszText, SIZEOF( group->cl.items[i]->szText ));
	return i;
}

static void AddContactToGroup(struct ClcData *dat, struct ClcGroup *group, HANDLE hContact)
{
	char *szProto;
	WORD apparentMode;
	DWORD idleMode;

	int i, index = -1;

	dat->needsResort = 1;
	for (i = group->cl.count - 1; i >= 0; i--) {
		if (group->cl.items[i]->hContact == hContact )
			return;

		if ( index == -1 )
			if (group->cl.items[i]->type != CLCIT_INFO || !(group->cl.items[i]->flags & CLCIIF_BELOWCONTACTS))
				index = i;
	}

	i = cli.pfnAddItemToGroup(group, index + 1);
	szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	group->cl.items[i]->type = CLCIT_CONTACT;
	group->cl.items[i]->iImage = CallService(MS_CLIST_GETCONTACTICON, (WPARAM) hContact, 0);
	group->cl.items[i]->hContact = hContact;
	group->cl.items[i]->proto = szProto;
	if (szProto != NULL && !cli.pfnIsHiddenMode(dat, DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE)))
		group->cl.items[i]->flags |= CONTACTF_ONLINE;
	apparentMode = szProto != NULL ? DBGetContactSettingWord(hContact, szProto, "ApparentMode", 0) : 0;
	if (apparentMode == ID_STATUS_OFFLINE)
		group->cl.items[i]->flags |= CONTACTF_INVISTO;
	else if (apparentMode == ID_STATUS_ONLINE)
		group->cl.items[i]->flags |= CONTACTF_VISTO;
	else if (apparentMode)
		group->cl.items[i]->flags |= CONTACTF_VISTO | CONTACTF_INVISTO;
	if (DBGetContactSettingByte(hContact, "CList", "NotOnList", 0))
		group->cl.items[i]->flags |= CONTACTF_NOTONLIST;
	idleMode = szProto != NULL ? DBGetContactSettingDword(hContact, szProto, "IdleTS", 0) : 0;
	if (idleMode)
		group->cl.items[i]->flags |= CONTACTF_IDLE;
	lstrcpyn(group->cl.items[i]->szText, cli.pfnGetContactDisplayName(hContact,0), SIZEOF(group->cl.items[i]->szText));
}

void fnAddContactToTree(HWND hwnd, struct ClcData *dat, HANDLE hContact, int updateTotalCount, int checkHideOffline)
{
	struct ClcGroup *group;
	DBVARIANT dbv;
	DWORD style = GetWindowLong(hwnd, GWL_STYLE);
	WORD status;
	char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);

	dat->needsResort = 1;
	if (style & CLS_NOHIDEOFFLINE)
		checkHideOffline = 0;
	if (checkHideOffline) {
		if (szProto == NULL)
			status = ID_STATUS_OFFLINE;
		else
			status = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
	}

	if ( DBGetContactSettingTString(hContact, "CList", "Group", &dbv))
		group = &dat->list;
	else {
		group = cli.pfnAddGroup(hwnd, dat, dbv.ptszVal, (DWORD) - 1, 0, 0);
		if (group == NULL) {
			int i, len;
			DWORD groupFlags;
			TCHAR *szGroupName;
			if (!(style & CLS_HIDEEMPTYGROUPS)) {
				free(dbv.pszVal);
				return;
			}
			if (checkHideOffline && cli.pfnIsHiddenMode(dat, status)) {
				for (i = 1;; i++) {
					szGroupName = cli.pfnGetGroupName(i, &groupFlags);
					if (szGroupName == NULL) {
						free(dbv.pszVal);
						return;
					}           //never happens
					if (!lstrcmp(szGroupName, dbv.ptszVal))
						break;
				}
				if (groupFlags & GROUPF_HIDEOFFLINE) {
					free(dbv.pszVal);
					return;
				}
			}
			for (i = 1;; i++) {
				szGroupName = cli.pfnGetGroupName(i, &groupFlags);
				if (szGroupName == NULL) {
					free(dbv.pszVal);
					return;
				}               //never happens
				if (!lstrcmp(szGroupName, dbv.ptszVal))
					break;
				len = lstrlen(szGroupName);
				if (!_tcsncmp(szGroupName, dbv.ptszVal, len) && dbv.ptszVal[len] == '\\')
					cli.pfnAddGroup(hwnd, dat, szGroupName, groupFlags, i, 1);
			}
			group = cli.pfnAddGroup(hwnd, dat, dbv.ptszVal, groupFlags, i, 1);
		}
		free(dbv.pszVal);
	}
	if (checkHideOffline) {
		if (cli.pfnIsHiddenMode(dat, status) && (style & CLS_HIDEOFFLINE || group->hideOffline)) {
			if (updateTotalCount)
				group->totalMembers++;
			return;
		}
	}
	AddContactToGroup(dat, group, hContact);
	if (updateTotalCount)
		group->totalMembers++;
}

struct ClcGroup* fnRemoveItemFromGroup(HWND hwnd, struct ClcGroup *group, struct ClcContact *contact, int updateTotalCount)
{
	int iContact;
	if ( !List_GetIndex(( SortedList* )&group->cl, contact, &iContact ))
		return group;

	if (contact->type == CLCIT_GROUP) {
		cli.pfnFreeGroup(contact->group);
		free(contact->group);
	}

	if (updateTotalCount && contact->type == CLCIT_CONTACT)
		group->totalMembers--;

	List_Remove(( SortedList* )&group->cl, iContact );
	if ((GetWindowLong(hwnd, GWL_STYLE) & CLS_HIDEEMPTYGROUPS) && group->cl.count == 0) {
		int i;
		if (group->parent == NULL)
			return group;
		for (i = 0; i < group->parent->cl.count; i++)
			if (group->parent->cl.items[i]->type == CLCIT_GROUP && group->parent->cl.items[i]->groupId == group->groupId)
				break;
		if (i == group->parent->cl.count)
			return group;       //never happens
		return cli.pfnRemoveItemFromGroup(hwnd, group->parent, group->parent->cl.items[i], 0);
	}
	return group;
}

void fnDeleteItemFromTree(HWND hwnd, HANDLE hItem)
{
	struct ClcContact *contact;
	struct ClcGroup *group;
	struct ClcData *dat = (struct ClcData *) GetWindowLong(hwnd, 0);

	dat->needsResort = 1;
	if (!cli.pfnFindItem(hwnd, dat, hItem, &contact, &group, NULL)) {
		DBVARIANT dbv;
		int i, nameOffset;
		if (!IsHContactContact(hItem))
			return;
		if (DBGetContactSettingTString(hItem, "CList", "Group", &dbv))
			return;

		//decrease member counts of all parent groups too
		group = &dat->list;
		nameOffset = 0;
		for (i = 0;; i++) {
			if (group->scanIndex == group->cl.count)
				break;
			if (group->cl.items[i]->type == CLCIT_GROUP) {
				int len = lstrlen(group->cl.items[i]->szText);
				if (!_tcsncmp(group->cl.items[i]->szText, dbv.ptszVal + nameOffset, len) && 
					 (dbv.ptszVal[nameOffset + len] == '\\' || dbv.ptszVal[nameOffset + len] == '\0')) {
					group->totalMembers--;
					if (dbv.ptszVal[nameOffset + len] == '\0')
						break;
				}
			}
		}
		free(dbv.ptszVal);
	}
	else
		cli.pfnRemoveItemFromGroup(hwnd, group, contact, 1);
}

void fnRebuildEntireList(HWND hwnd, struct ClcData *dat)
{
	char *szProto;
	DWORD style = GetWindowLong(hwnd, GWL_STYLE);
	HANDLE hContact;
	struct ClcGroup *group;
	DBVARIANT dbv;

	dat->list.expanded = 1;
	dat->list.hideOffline = DBGetContactSettingByte(NULL, "CLC", "HideOfflineRoot", 0);
	dat->list.cl.count = dat->list.cl.limit = 0;
	dat->selection = -1;
	{
		int i;
		TCHAR *szGroupName;
		DWORD groupFlags;

		for (i = 1;; i++) {
			szGroupName = cli.pfnGetGroupName(i, &groupFlags);
			if (szGroupName == NULL)
				break;
			cli.pfnAddGroup(hwnd, dat, szGroupName, groupFlags, i, 0);
		}
	}

	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact) {
		if (style & CLS_SHOWHIDDEN || !DBGetContactSettingByte(hContact, "CList", "Hidden", 0)) {
			if (DBGetContactSettingTString(hContact, "CList", "Group", &dbv))
				group = &dat->list;
			else {
				group = cli.pfnAddGroup(hwnd, dat, dbv.ptszVal, (DWORD) - 1, 0, 0);
				free(dbv.ptszVal);
			}

			if (group != NULL) {
				group->totalMembers++;
				if (!(style & CLS_NOHIDEOFFLINE) && (style & CLS_HIDEOFFLINE || group->hideOffline)) {
					szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
					if (szProto == NULL) {
						if (!cli.pfnIsHiddenMode(dat, ID_STATUS_OFFLINE))
							AddContactToGroup(dat, group, hContact);
					}
					else if (!cli.pfnIsHiddenMode(dat, DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE)))
						AddContactToGroup(dat, group, hContact);
				}
				else
					AddContactToGroup(dat, group, hContact);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}

	if (style & CLS_HIDEEMPTYGROUPS) {
		group = &dat->list;
		group->scanIndex = 0;
		for (;;) {
			if (group->scanIndex == group->cl.count) {
				group = group->parent;
				if (group == NULL)
					break;
			}
			else if (group->cl.items[group->scanIndex]->type == CLCIT_GROUP) {
				if (group->cl.items[group->scanIndex]->group->cl.count == 0) {
					group = cli.pfnRemoveItemFromGroup(hwnd, group, group->cl.items[group->scanIndex], 0);
				}
				else {
					group = group->cl.items[group->scanIndex]->group;
					group->scanIndex = 0;
				}
				continue;
			}
			group->scanIndex++;
		}
	}

	cli.pfnSortCLC(hwnd, dat, 0);
}

int fnGetGroupContentsCount(struct ClcGroup *group, int visibleOnly)
{
	int count = group->cl.count;
	struct ClcGroup *topgroup = group;

	group->scanIndex = 0;
	for (;;) {
		if (group->scanIndex == group->cl.count) {
			if (group == topgroup)
				break;
			group = group->parent;
		}
		else if (group->cl.items[group->scanIndex]->type == CLCIT_GROUP && (!visibleOnly || group->cl.items[group->scanIndex]->group->expanded)) {
			group = group->cl.items[group->scanIndex]->group;
			group->scanIndex = 0;
			count += group->cl.count;
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

static int __cdecl ContactSortProc(const struct ClcContact **contact1, const struct ClcContact **contact2)
{
	int result = CallService(MS_CLIST_CONTACTSCOMPARE, (WPARAM) contact1[0]->hContact, (LPARAM) contact2[0]->hContact);
	if (result)
		return result;
	//nothing to distinguish them, so make sure they stay in the same order
	return (int) contact2[0]->hContact - (int) contact1[0]->hContact;
}

static void InsertionSort(struct ClcContact **pContactArray, int nArray, int (*CompareProc) (const void *, const void *))
{
	int i, j;
	struct ClcContact* testElement;

	for (i = 1; i < nArray; i++) {
		if (CompareProc(&pContactArray[i - 1], &pContactArray[i]) > 0) {
			testElement = pContactArray[i];
			for (j = i - 2; j >= 0; j--)
				if (CompareProc(&pContactArray[j], &testElement) <= 0)
					break;
			j++;
			memmove(&pContactArray[j + 1], &pContactArray[j], sizeof(void*) * (i - j));
			pContactArray[j] = testElement;
}	}	}

static void SortGroup(struct ClcData *dat, struct ClcGroup *group, int useInsertionSort)
{
	int i, sortCount;

	for (i = group->cl.count - 1; i >= 0; i--) {
		if (group->cl.items[i]->type == CLCIT_DIVIDER) {
			free( group->cl.items[i] );
			List_Remove(( SortedList* )&group->cl, i );
	}	}

	for (i = 0; i < group->cl.count; i++)
		if (group->cl.items[i]->type != CLCIT_INFO)
			break;
	if (i > group->cl.count - 2)
		return;
	if (group->cl.items[i]->type == CLCIT_GROUP) {
		if (dat->exStyle & CLS_EX_SORTGROUPSALPHA) {
			for (sortCount = 0; i + sortCount < group->cl.count; sortCount++)
				if (group->cl.items[i + sortCount]->type != CLCIT_GROUP)
					break;
			qsort(group->cl.items + i, sortCount, sizeof(void*), GroupSortProc);
			i = i + sortCount;
		}
		for (; i < group->cl.count; i++)
			if (group->cl.items[i]->type == CLCIT_CONTACT)
				break;
		if (group->cl.count - i < 2)
			return;
	}
	for (sortCount = 0; i + sortCount < group->cl.count; sortCount++)
		if (group->cl.items[i + sortCount]->type != CLCIT_CONTACT)
			break;
	if (useInsertionSort)
		InsertionSort(group->cl.items + i, sortCount, ContactSortProc);
	else
		qsort(group->cl.items + i, sortCount, sizeof(void*), ContactSortProc);
	if (dat->exStyle & CLS_EX_DIVIDERONOFF) {
		int prevContactOnline = 0;
		for (i = 0; i < group->cl.count; i++) {
			if (group->cl.items[i]->type != CLCIT_CONTACT)
				continue;
			if (group->cl.items[i]->flags & CONTACTF_ONLINE)
				prevContactOnline = 1;
			else {
				if (prevContactOnline) {
					i = cli.pfnAddItemToGroup(group, i);
					group->cl.items[i]->type = CLCIT_DIVIDER;
					lstrcpy(group->cl.items[i]->szText, TranslateT("Offline"));
				}
				break;
}	}	}	}

void fnSortCLC(HWND hwnd, struct ClcData *dat, int useInsertionSort)
{
	struct ClcContact *selcontact;
	struct ClcGroup *group = &dat->list, *selgroup;
	int dividers = dat->exStyle & CLS_EX_DIVIDERONOFF;
	HANDLE hSelItem;

	if ( dat->needsResort ) {
		if (cli.pfnGetRowByIndex(dat, dat->selection, &selcontact, NULL) == -1)
			hSelItem = NULL;
		else
			hSelItem = cli.pfnContactToHItem(selcontact);
		group->scanIndex = 0;
		SortGroup(dat, group, useInsertionSort);
		for (;;) {
			if (group->scanIndex == group->cl.count) {
				group = group->parent;
				if (group == NULL)
					break;
			}
			else if (group->cl.items[group->scanIndex]->type == CLCIT_GROUP) {
				group = group->cl.items[group->scanIndex]->group;
				group->scanIndex = 0;
				SortGroup(dat, group, useInsertionSort);
				continue;
			}
			group->scanIndex++;
		}
		if (hSelItem)
			if (cli.pfnFindItem(hwnd, dat, hSelItem, &selcontact, &selgroup, NULL)) {
				int idx;
				if ( List_GetIndex((SortedList*)&selgroup->cl,selcontact,&idx ))
					dat->selection = cli.pfnGetRowsPriorTo(&dat->list, selgroup, idx );
			}
	}
	dat->needsResort = 0;
	InvalidateRect(hwnd, NULL, FALSE);
	cli.pfnRecalcScrollBar(hwnd, dat);
}

struct SavedContactState_t
{
	HANDLE hContact;
	BYTE iExtraImage[MAXEXTRACOLUMNS];
	int checked;
};

struct SavedGroupState_t
{
	int groupId, expanded;
};

struct SavedInfoState_t
{
	int parentId;
	struct ClcContact contact;
};

void fnSaveStateAndRebuildList(HWND hwnd, struct ClcData *dat)
{
	NMCLISTCONTROL nm;
	int i, j;
	struct SavedGroupState_t *savedGroup = NULL;
	int savedGroupCount = 0, savedGroupAlloced = 0;
	struct SavedContactState_t *savedContact = NULL;
	int savedContactCount = 0, savedContactAlloced = 0;
	struct SavedInfoState_t *savedInfo = NULL;
	int savedInfoCount = 0, savedInfoAlloced = 0;
	struct ClcGroup *group;
	struct ClcContact *contact;

	cli.pfnHideInfoTip(hwnd, dat);
	KillTimer(hwnd, TIMERID_INFOTIP);
	KillTimer(hwnd, TIMERID_RENAME);
	cli.pfnEndRename(hwnd, dat, 1);

	dat->needsResort = 1;
	group = &dat->list;
	group->scanIndex = 0;
	for (;;) {
		if (group->scanIndex == group->cl.count) {
			group = group->parent;
			if (group == NULL)
				break;
		}
		else if (group->cl.items[group->scanIndex]->type == CLCIT_GROUP) {
			group = group->cl.items[group->scanIndex]->group;
			group->scanIndex = 0;
			if (++savedGroupCount > savedGroupAlloced) {
				savedGroupAlloced += 8;
				savedGroup = (struct SavedGroupState_t *) realloc(savedGroup, sizeof(struct SavedGroupState_t) * savedGroupAlloced);
			}
			savedGroup[savedGroupCount - 1].groupId = group->groupId;
			savedGroup[savedGroupCount - 1].expanded = group->expanded;
			continue;
		}
		else if (group->cl.items[group->scanIndex]->type == CLCIT_CONTACT) {
			if (++savedContactCount > savedContactAlloced) {
				savedContactAlloced += 16;
				savedContact = (struct SavedContactState_t *) realloc(savedContact, sizeof(struct SavedContactState_t) * savedContactAlloced);
			}
			savedContact[savedContactCount - 1].hContact = group->cl.items[group->scanIndex]->hContact;
			CopyMemory(savedContact[savedContactCount - 1].iExtraImage, group->cl.items[group->scanIndex]->iExtraImage,
				sizeof(group->cl.items[group->scanIndex]->iExtraImage));
			savedContact[savedContactCount - 1].checked = group->cl.items[group->scanIndex]->flags & CONTACTF_CHECKED;
		}
		else if (group->cl.items[group->scanIndex]->type == CLCIT_INFO) {
			if (++savedInfoCount > savedInfoAlloced) {
				savedInfoAlloced += 4;
				savedInfo = (struct SavedInfoState_t *) realloc(savedInfo, sizeof(struct SavedInfoState_t) * savedInfoAlloced);
			}
			if (group->parent == NULL)
				savedInfo[savedInfoCount - 1].parentId = -1;
			else
				savedInfo[savedInfoCount - 1].parentId = group->groupId;
			savedInfo[savedInfoCount - 1].contact = *group->cl.items[group->scanIndex];
		}
		group->scanIndex++;
	}

	cli.pfnFreeGroup(&dat->list);
	cli.pfnRebuildEntireList(hwnd, dat);

	group = &dat->list;
	group->scanIndex = 0;
	for (;;) {
		if (group->scanIndex == group->cl.count) {
			group = group->parent;
			if (group == NULL)
				break;
		}
		else if (group->cl.items[group->scanIndex]->type == CLCIT_GROUP) {
			group = group->cl.items[group->scanIndex]->group;
			group->scanIndex = 0;
			for (i = 0; i < savedGroupCount; i++)
				if (savedGroup[i].groupId == group->groupId) {
					group->expanded = savedGroup[i].expanded;
					break;
				}
				continue;
		}
		else if (group->cl.items[group->scanIndex]->type == CLCIT_CONTACT) {
			for (i = 0; i < savedContactCount; i++)
				if (savedContact[i].hContact == group->cl.items[group->scanIndex]->hContact) {
					CopyMemory(group->cl.items[group->scanIndex]->iExtraImage, savedContact[i].iExtraImage,
						SIZEOF(group->cl.items[group->scanIndex]->iExtraImage));
					if (savedContact[i].checked)
						group->cl.items[group->scanIndex]->flags |= CONTACTF_CHECKED;
					break;
				}
		}
		group->scanIndex++;
	}
	if (savedGroup)
		free(savedGroup);
	if (savedContact)
		free(savedContact);
	for (i = 0; i < savedInfoCount; i++) {
		if (savedInfo[i].parentId == -1)
			group = &dat->list;
		else {
			if (!cli.pfnFindItem(hwnd, dat, (HANDLE) (savedInfo[i].parentId | HCONTACT_ISGROUP), &contact, NULL, NULL))
				continue;
			group = contact->group;
		}
		j = cli.pfnAddInfoItemToGroup(group, savedInfo[i].contact.flags, _T(""));
		*group->cl.items[j] = savedInfo[i].contact;
	}
	if (savedInfo)
		free(savedInfo);
	cli.pfnRecalculateGroupCheckboxes(hwnd, dat);

	cli.pfnRecalcScrollBar(hwnd, dat);
	nm.hdr.code = CLN_LISTREBUILT;
	nm.hdr.hwndFrom = hwnd;
	nm.hdr.idFrom = GetDlgCtrlID(hwnd);
	SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) & nm);
}
