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
#include "m_clc.h"
#include "clc.h"
#include "clist.h"

//routines for managing adding/removal of items in the list, including sorting

extern int CompareContacts(WPARAM wParam,LPARAM lParam);
extern void ClearClcContactCache(struct ClcData *dat,HANDLE hContact);

static int AddItemToGroup(struct ClcGroup *group,int iAboveItem)
{
	if(++group->contactCount>group->allocedCount) {
		group->allocedCount+=GROUP_ALLOCATE_STEP;
		group->contact=(struct ClcContact*)mir_realloc(group->contact,sizeof(struct ClcContact)*group->allocedCount);
	}
	memmove(group->contact+iAboveItem+1,group->contact+iAboveItem,sizeof(struct ClcContact)*(group->contactCount-iAboveItem-1));
	group->contact[iAboveItem].type=CLCIT_DIVIDER;
	group->contact[iAboveItem].flags=0;
	memset(group->contact[iAboveItem].iExtraImage,0xFF,sizeof(group->contact[iAboveItem].iExtraImage));
	group->contact[iAboveItem].szText[0]='\0';
	
	ClearRowByIndexCache();
	return iAboveItem;
}

struct ClcGroup *AddGroup(HWND hwnd,struct ClcData *dat,const char *szName,DWORD flags,int groupId,int calcTotalMembers)
{
	char *pBackslash,*pNextField,szThisField[sizeof(dat->list.contact[0].szText)];
	struct ClcGroup *group=&dat->list;
	int i,compareResult;

	ClearRowByIndexCache();	
	dat->NeedResort=1;

	if(!(GetWindowLong(hwnd,GWL_STYLE)&CLS_USEGROUPS)) return &dat->list;
	pNextField=(char*)szName;
	do {
		pBackslash=strchr(pNextField,'\\');
		if(pBackslash==NULL) {
			lstrcpyn(szThisField,pNextField,sizeof(szThisField));
			pNextField=NULL;
		}
		else {
			lstrcpyn(szThisField,pNextField,min(sizeof(szThisField),pBackslash-pNextField+1));
			pNextField=pBackslash+1;
		}
		compareResult=1;
		for(i=0;i<group->contactCount;i++) {
			if(group->contact[i].type==CLCIT_CONTACT) break;
			if(group->contact[i].type!=CLCIT_GROUP) continue;
			compareResult=lstrcmp(szThisField,group->contact[i].szText);
			if(compareResult==0) {
				if(pNextField==NULL && flags!=(DWORD)-1) {
					group->contact[i].groupId=(WORD)groupId;
					group=group->contact[i].group;
					group->expanded=(flags&GROUPF_EXPANDED)!=0;
					group->hideOffline=(flags&GROUPF_HIDEOFFLINE)!=0;
					group->groupId=groupId;
				}
				else group=group->contact[i].group;
				break;
			}
			if(pNextField==NULL && group->contact[i].groupId==0) break;
			if(groupId && group->contact[i].groupId>groupId) break;
		}
		if(compareResult) {
			if(groupId==0) return NULL;
			i=AddItemToGroup(group,i);
			group->contact[i].type=CLCIT_GROUP;
			lstrcpyn(group->contact[i].szText,szThisField,sizeof(group->contact[i].szText));
			group->contact[i].groupId=(WORD)(pNextField?0:groupId);
			group->contact[i].group=(struct ClcGroup*)mir_alloc(sizeof(struct ClcGroup));
			group->contact[i].group->parent=group;
			group=group->contact[i].group;
			group->allocedCount=group->contactCount=0;
			group->contact=NULL;
			if(flags==(DWORD)-1 || pNextField!=NULL) {
				group->expanded=0;
				group->hideOffline=0;
			}
			else {
				group->expanded=(flags&GROUPF_EXPANDED)!=0;
				group->hideOffline=(flags&GROUPF_HIDEOFFLINE)!=0;
			}
			group->groupId=pNextField?0:groupId;
			group->totalMembers=0;
			if(flags!=(DWORD)-1 && pNextField==NULL && calcTotalMembers) {
				HANDLE hContact;
				//DBVARIANT dbv;
				int tick;
				char buf[256];

				DWORD style=GetWindowLong(hwnd,GWL_STYLE);

				hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);

				tick=GetTickCount();
				while(hContact) {
					pdisplayNameCacheEntry cacheEntry;
					cacheEntry=GetContactFullCacheEntry(hContact);

					if(cacheEntry->szGroup!=NULL&&strlen(cacheEntry->szGroup)!=0) {
						if(!lstrcmp(cacheEntry->szGroup,szName) && (style&CLS_SHOWHIDDEN || !cacheEntry->Hidden))
							group->totalMembers++;
						//mir_free(dbv.pszVal);
					}
					hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
				}
				tick=GetTickCount()-tick;
				sprintf(buf,"AddGroup Finds %d\r\n",tick);
				OutputDebugString(buf);
			}
		}
	} while(pNextField);
	
	ClearRowByIndexCache();
	return group;
}

void FreeGroup(struct ClcGroup *group)
{
	int i;
	for(i=0;i<group->contactCount;i++) {
		if(group->contact[i].type==CLCIT_GROUP) {
			FreeGroup(group->contact[i].group);
			mir_free(group->contact[i].group);
		}
	}
	if(group->allocedCount)
		mir_free(group->contact);
	group->allocedCount=0;
	group->contact=NULL;
	group->contactCount=0;
	
	ClearRowByIndexCache();
}

static int iInfoItemUniqueHandle=0;
int AddInfoItemToGroup(struct ClcGroup *group,int flags,const char *pszText)
{
	int i=0;

	if(flags&CLCIIF_BELOWCONTACTS)
		i=group->contactCount;
	else if(flags&CLCIIF_BELOWGROUPS) {
		for(;i<group->contactCount;i++)
			if(group->contact[i].type==CLCIT_CONTACT) break;
	}
	else
		for(;i<group->contactCount;i++)
			if(group->contact[i].type!=CLCIT_INFO) break;
	i=AddItemToGroup(group,i);
	iInfoItemUniqueHandle=(iInfoItemUniqueHandle+1)&0xFFFF;
	if(iInfoItemUniqueHandle==0) ++iInfoItemUniqueHandle;
	group->contact[i].type=CLCIT_INFO;
	group->contact[i].flags=(BYTE)flags;
	group->contact[i].hContact=(HANDLE)++iInfoItemUniqueHandle;
	lstrcpyn(group->contact[i].szText,pszText,sizeof(group->contact[i].szText));
	ClearRowByIndexCache();
	return i;
}

static void AddContactToGroup(struct ClcData *dat,struct ClcGroup *group,pdisplayNameCacheEntry cacheEntry)
{
	char *szProto;
	WORD apparentMode;
	DWORD idleMode;
	HANDLE hContact;

	int i;
	
	hContact=cacheEntry->hContact;
	//ClearClcContactCache(hContact);

	dat->NeedResort=1;
	for(i=group->contactCount-1;i>=0;i--)
		if(group->contact[i].type!=CLCIT_INFO || !(group->contact[i].flags&CLCIIF_BELOWCONTACTS)) break;
	i=AddItemToGroup(group,i+1);
	group->contact[i].type=CLCIT_CONTACT;
	group->contact[i].iImage=CallService(MS_CLIST_GETCONTACTICON,(WPARAM)hContact,0);
	group->contact[i].hContact=hContact;
	
	//cacheEntry->ClcContact=&(group->contact[i]);
	//SetClcContactCacheItem(dat,hContact,&(group->contact[i]));

	szProto=cacheEntry->szProto;
	if(szProto!=NULL&&!IsHiddenMode(dat,cacheEntry->status))
		group->contact[i].flags|=CONTACTF_ONLINE;
	apparentMode=szProto!=NULL?cacheEntry->ApparentMode:0;
	if(apparentMode==ID_STATUS_OFFLINE)	group->contact[i].flags|=CONTACTF_INVISTO;
	else if(apparentMode==ID_STATUS_ONLINE) group->contact[i].flags|=CONTACTF_VISTO;
	else if(apparentMode) group->contact[i].flags|=CONTACTF_VISTO|CONTACTF_INVISTO;
	if(cacheEntry->NotOnList) group->contact[i].flags|=CONTACTF_NOTONLIST;
	idleMode=szProto!=NULL?cacheEntry->IdleTS:0;
	if (idleMode) group->contact[i].flags|=CONTACTF_IDLE;

	lstrcpyn(group->contact[i].szText,cacheEntry->name,sizeof(group->contact[i].szText));
	group->contact[i].proto = szProto;
	
	ClearRowByIndexCache();
}

void AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline)
{
	struct ClcGroup *group;
	pdisplayNameCacheEntry cacheEntry;
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);
	WORD status;
	char *szProto;
	
	if (FindItem(hwnd,dat,hContact,NULL,NULL,NULL)==1){return;};	
	cacheEntry=GetContactFullCacheEntry(hContact);
	szProto=cacheEntry->szProto;


	//char *szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
	
	dat->NeedResort=1;
	ClearRowByIndexCache();
	ClearClcContactCache(dat,hContact);
	
	if(style&CLS_NOHIDEOFFLINE) checkHideOffline=0;
	if(checkHideOffline) {
		if(szProto==NULL) status=ID_STATUS_OFFLINE;
		else status=cacheEntry->status;
	}

	if(strlen(cacheEntry->szGroup)==0)
		group=&dat->list;
	else {
		group=AddGroup(hwnd,dat,cacheEntry->szGroup,(DWORD)-1,0,0);
		if(group==NULL) {
			int i,len;
			DWORD groupFlags;
			char *szGroupName;
			if(!(style&CLS_HIDEEMPTYGROUPS)) {/*mir_free(dbv.pszVal);*/return;}
			if(checkHideOffline && IsHiddenMode(dat,status)) {
				for(i=1;;i++) {
					szGroupName=(char*)CallService(MS_CLIST_GROUPGETNAME2,i,(LPARAM)&groupFlags);
					if(szGroupName==NULL) {/*mir_free(dbv.pszVal);*/ return;}   //never happens
					if(!lstrcmp(szGroupName,cacheEntry->szGroup)) break;
				}
				if(groupFlags&GROUPF_HIDEOFFLINE) {/*mir_free(dbv.pszVal);*/ return;}
			}
			for(i=1;;i++) {
				szGroupName=(char*)CallService(MS_CLIST_GROUPGETNAME2,i,(LPARAM)&groupFlags);
				if(szGroupName==NULL) {/*mir_free(dbv.pszVal);*/ return;}   //never happens
				if(!lstrcmp(szGroupName,cacheEntry->szGroup)) break;
				len=lstrlen(szGroupName);
				if(!strncmp(szGroupName,cacheEntry->szGroup,len) && cacheEntry->szGroup[len]=='\\')
					AddGroup(hwnd,dat,szGroupName,groupFlags,i,1);
			}
			group=AddGroup(hwnd,dat,cacheEntry->szGroup,groupFlags,i,1);
		}
	//	mir_free(dbv.pszVal);
	}
	if(checkHideOffline) {
		if(IsHiddenMode(dat,status) && (style&CLS_HIDEOFFLINE || group->hideOffline)) {
			if(updateTotalCount) group->totalMembers++;
			return;
		}
	}
	AddContactToGroup(dat,group,cacheEntry);
	if(updateTotalCount) group->totalMembers++;
	ClearRowByIndexCache();
}

struct ClcGroup *RemoveItemFromGroup(HWND hwnd,struct ClcGroup *group,struct ClcContact *contact,int updateTotalCount)
{
	int iContact;
	struct ClcData* dat=(struct ClcData*)GetWindowLong(hwnd,0);
	
	ClearRowByIndexCache();
	if(contact->type==CLCIT_CONTACT) ClearClcContactCache(dat,contact->hContact);

	iContact=((unsigned)contact-(unsigned)group->contact)/sizeof(struct ClcContact);
	if(iContact>=group->contactCount) return group;
	if(contact->type==CLCIT_GROUP) {
		FreeGroup(contact->group);
		mir_free(contact->group);
	}
	group->contactCount--;
	if(updateTotalCount && contact->type==CLCIT_CONTACT) group->totalMembers--;
	memmove(group->contact+iContact,group->contact+iContact+1,sizeof(struct ClcContact)*(group->contactCount-iContact));
	if((GetWindowLong(hwnd,GWL_STYLE)&CLS_HIDEEMPTYGROUPS) && group->contactCount==0) {
		int i;
		if(group->parent==NULL) return group;
		for(i=0;i<group->parent->contactCount;i++)
			if(group->parent->contact[i].groupId==group->groupId) break;
		if(i==group->parent->contactCount) return group;  //never happens
		return RemoveItemFromGroup(hwnd,group->parent,&group->parent->contact[i],0);
	}

	
	ClearRowByIndexCache();
	return group;
}

void DeleteItemFromTree(HWND hwnd,HANDLE hItem)
{
	struct ClcContact *contact;
	struct ClcGroup *group;
	struct ClcData *dat=(struct ClcData*)GetWindowLong(hwnd,0);
	
	ClearRowByIndexCache();
	dat->NeedResort=1;
	
	if(!FindItem(hwnd,dat,hItem,&contact,&group,NULL)) {
		DBVARIANT dbv;
		int i,nameOffset;
		if(!IsHContactContact(hItem)) return;
		ClearClcContactCache(dat,hItem);

		if(DBGetContactSetting(hItem,"CList","Group",&dbv)) return;

		//decrease member counts of all parent groups too
		group=&dat->list;
		nameOffset=0;
		for(i=0;;i++) {
			if(group->scanIndex==group->contactCount) break;
			if(group->contact[i].type==CLCIT_GROUP) {
				int len=lstrlen(group->contact[i].szText);
				if(!strncmp(group->contact[i].szText,dbv.pszVal+nameOffset,len) && (dbv.pszVal[nameOffset+len]=='\\' || dbv.pszVal[nameOffset+len]=='\0')) {
					group->totalMembers--;
					if(dbv.pszVal[nameOffset+len]=='\0') break;
				}
			}
		}
		mir_free(dbv.pszVal);
	}
	else RemoveItemFromGroup(hwnd,group,contact,1);

	ClearRowByIndexCache();
}

void RebuildEntireList(HWND hwnd,struct ClcData *dat)
{
//	char *szProto;
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);
	HANDLE hContact;
	struct ClcGroup *group;
	//DBVARIANT dbv;
	int tick=GetTickCount();

	ClearRowByIndexCache();
	ClearClcContactCache(dat,INVALID_HANDLE_VALUE);

	dat->list.expanded=1;
	dat->list.hideOffline=DBGetContactSettingByte(NULL,"CLC","HideOfflineRoot",0);
	dat->list.contactCount=0;
	dat->list.totalMembers=0;
	dat->NeedResort=1;
	dat->selection=-1;
	dat->HiLightMode=DBGetContactSettingByte(NULL,"CLC","HiLightMode",0);
	{
		int i;
		char *szGroupName;
		DWORD groupFlags;

		for(i=1;;i++) {
			szGroupName=(char*)CallService(MS_CLIST_GROUPGETNAME2,i,(LPARAM)&groupFlags);
			if(szGroupName==NULL) break;
			AddGroup(hwnd,dat,szGroupName,groupFlags,i,0);
		}
	}

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact) {
		
		pdisplayNameCacheEntry cacheEntry;
		
		cacheEntry=GetContactFullCacheEntry(hContact);
		//cacheEntry->ClcContact=NULL;
		ClearClcContactCache(dat,hContact);
		if (cacheEntry==NULL)
		{
			MessageBox(0,"Fail To Get CacheEntry for hContact","!!!!!!!!",0);
		}
		

		if(style&CLS_SHOWHIDDEN || !cacheEntry->Hidden) {
			if(strlen(cacheEntry->szGroup)==0)
				group=&dat->list;
			else {
				group=AddGroup(hwnd,dat,cacheEntry->szGroup,(DWORD)-1,0,0);
				//mir_free(dbv.pszVal);
			}

			if(group!=NULL) {
				group->totalMembers++;
				if(!(style&CLS_NOHIDEOFFLINE) && (style&CLS_HIDEOFFLINE || group->hideOffline)) {
					//szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
					if(cacheEntry->szProto==NULL) {
						if(!IsHiddenMode(dat,ID_STATUS_OFFLINE))
							AddContactToGroup(dat,group,cacheEntry);
					}
					else
						if(!IsHiddenMode(dat,cacheEntry->status))
							AddContactToGroup(dat,group,cacheEntry);
				}
				else AddContactToGroup(dat,group,cacheEntry);
			}
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}

	if(style&CLS_HIDEEMPTYGROUPS) {
		group=&dat->list;
		group->scanIndex=0;
		for(;;) {
			if(group->scanIndex==group->contactCount) {
				group=group->parent;
				if(group==NULL) break;
			}
			else if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
				if(group->contact[group->scanIndex].group->contactCount==0) {
					group=RemoveItemFromGroup(hwnd,group,&group->contact[group->scanIndex],0);
				}
				else {
					group=group->contact[group->scanIndex].group;
					group->scanIndex=0;
				}
				continue;
			}
			group->scanIndex++;
		}
	}

	SortCLC(hwnd,dat,0);
	tick=GetTickCount()-tick;
	{
	char buf[255];
	//sprintf(buf,"%s %s took %i ms",__FILE__,__LINE__,tick);
	sprintf(buf,"RebuildEntireList %d \r\n",tick);

	OutputDebugString(buf);
	DBWriteContactSettingDword((HANDLE)0,"CLUI","PF:Last RebuildEntireList Time:",tick);
	}	
}

int GetGroupContentsCount(struct ClcGroup *group,int visibleOnly)
{
	int count=group->contactCount;
	struct ClcGroup *topgroup=group;

	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->contactCount) {
			if(group==topgroup) break;
			group=group->parent;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_GROUP && (!visibleOnly || group->contact[group->scanIndex].group->expanded)) {
			group=group->contact[group->scanIndex].group;
			group->scanIndex=0;
			count+=group->contactCount;
			continue;
		}
		group->scanIndex++;
	}
	return count;
}

static int __cdecl GroupSortProc(const struct ClcContact *contact1,const struct ClcContact *contact2)
{
	return lstrcmpi(contact1->szText,contact2->szText);
	
}

static int __cdecl ContactSortProc(const struct ClcContact *contact1,const struct ClcContact *contact2)
{
	int result;

	//result=CallService(MS_CLIST_CONTACTSCOMPARE,(WPARAM)contact1->hContact,(LPARAM)contact2->hContact);
	result=CompareContacts((WPARAM)contact1->hContact,(LPARAM)contact2->hContact);
	if(result) return result;
	//nothing to distinguish them, so make sure they stay in the same order
	return (int)contact2->hContact-(int)contact1->hContact;

	return 0;
}

static void InsertionSort(struct ClcContact *pContactArray,int nArray,int (*CompareProc)(const void*,const void*))
{
	int i,j;
	struct ClcContact testElement;

	for(i=1;i<nArray;i++) {
		if(CompareProc(&pContactArray[i-1],&pContactArray[i])>0) {
			testElement=pContactArray[i];
			for(j=i-2;j>=0;j--)
				if(CompareProc(&pContactArray[j],&testElement)<=0) break;
			j++;
			memmove(&pContactArray[j+1],&pContactArray[j],sizeof(struct ClcContact)*(i-j));
			pContactArray[j]=testElement;
		}
	}
}

static void SortGroup(struct ClcData *dat,struct ClcGroup *group,int useInsertionSort)
{
	int i,sortCount;

	for(i=group->contactCount-1;i>=0;i--) {
		if(group->contact[i].type==CLCIT_DIVIDER) {
			group->contactCount--;
			memmove(&group->contact[i],&group->contact[i+1],sizeof(struct ClcContact)*(group->contactCount-i));
		}
	}
	for(i=0;i<group->contactCount;i++)
		if(group->contact[i].type!=CLCIT_INFO) break;
	if(i>group->contactCount-2) return;
	if(group->contact[i].type==CLCIT_GROUP) {
		if(dat->exStyle&CLS_EX_SORTGROUPSALPHA) {
			for(sortCount=0;i+sortCount<group->contactCount;sortCount++)
				if(group->contact[i+sortCount].type!=CLCIT_GROUP) break;
			qsort(group->contact+i,sortCount,sizeof(struct ClcContact),GroupSortProc);
			i=i+sortCount;
		}
		for(;i<group->contactCount;i++)
			if(group->contact[i].type==CLCIT_CONTACT) break;
		if(group->contactCount-i<2) return;
	}
	for(sortCount=0;i+sortCount<group->contactCount;sortCount++)
		if(group->contact[i+sortCount].type!=CLCIT_CONTACT) break;
	if(useInsertionSort) InsertionSort(group->contact+i,sortCount,ContactSortProc);
	else qsort(group->contact+i,sortCount,sizeof(struct ClcContact),ContactSortProc);
	if(dat->exStyle&CLS_EX_DIVIDERONOFF) {
		int prevContactOnline=0;
		for(i=0;i<group->contactCount;i++) {
			if(group->contact[i].type!=CLCIT_CONTACT) continue;
			if(group->contact[i].flags&CONTACTF_ONLINE) prevContactOnline=1;
			else {
				if(prevContactOnline) {
					i=AddItemToGroup(group,i);
					group->contact[i].type=CLCIT_DIVIDER;
					lstrcpy(group->contact[i].szText,Translate("Offline"));
				}
				break;
			}
		}
	}
	ClearRowByIndexCache();
}

void SortCLC(HWND hwnd,struct ClcData *dat,int useInsertionSort)
{
	struct ClcContact *selcontact;
	struct ClcGroup *group=&dat->list,*selgroup;
	int dividers=dat->exStyle&CLS_EX_DIVIDERONOFF;
	HANDLE hSelItem;
	int tick=GetTickCount();
	if (dat->NeedResort==1)
	{

		if(GetRowByIndex(dat,dat->selection,&selcontact,NULL)==-1) hSelItem=NULL;
		else hSelItem=ContactToHItem(selcontact);
		group->scanIndex=0;
		
		SortGroup(dat,group,useInsertionSort);
		
		for(;;) {
			if(group->scanIndex==group->contactCount) {
				group=group->parent;
				if(group==NULL) break;
			}
			else if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
				group=group->contact[group->scanIndex].group;
				group->scanIndex=0;
				SortGroup(dat,group,useInsertionSort);
				continue;
			}
			group->scanIndex++;
		}
		
		ClearClcContactCache(dat,INVALID_HANDLE_VALUE);

		if(hSelItem)
			if(FindItem(hwnd,dat,hSelItem,&selcontact,&selgroup,NULL))
				dat->selection=GetRowsPriorTo(&dat->list,selgroup,selcontact-selgroup->contact);
		
		
		RecalcScrollBar(hwnd,dat);
		ClearRowByIndexCache();
	}else
	{
		//OutputDebugString("Not need to sort\r\n");
	};
	InvalidateRect(hwnd,NULL,FALSE);
	dat->NeedResort=0;

	tick=GetTickCount()-tick;
#ifdef _DEBUG
	{
	char buf[255];
	//sprintf(buf,"%s %s took %i ms",__FILE__,__LINE__,tick);
		if (tick>5) 
		{
			sprintf(buf,"SortCLC %d \r\n",tick);
			OutputDebugString(buf);
			DBWriteContactSettingDword((HANDLE)0,"CLUI","PF:Last SortCLC Time:",tick);
		}
	}
#endif	
}

struct SavedContactState_t {
	HANDLE hContact;
	BYTE iExtraImage[MAXEXTRACOLUMNS];
	int checked;
};

struct SavedGroupState_t {
	int groupId,expanded;
};

struct SavedInfoState_t {
	int parentId;
	struct ClcContact contact;
};

void SaveStateAndRebuildList(HWND hwnd,struct ClcData *dat)
{
	NMCLISTCONTROL nm;
	int i,j;
	struct SavedGroupState_t *savedGroup=NULL;
	int savedGroupCount=0,savedGroupAlloced=0;
	struct SavedContactState_t *savedContact=NULL;
	int savedContactCount=0,savedContactAlloced=0;
	struct SavedInfoState_t *savedInfo=NULL;
	int savedInfoCount=0,savedInfoAlloced=0;
	struct ClcGroup *group;
	struct ClcContact *contact;

	int tick=GetTickCount();
	int allocstep=1024;
	HideInfoTip(hwnd,dat);
	KillTimer(hwnd,TIMERID_INFOTIP);
	KillTimer(hwnd,TIMERID_RENAME);
	EndRename(hwnd,dat,1);

	group=&dat->list;
	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->contactCount) {
			group=group->parent;
			if(group==NULL) break;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
			group=group->contact[group->scanIndex].group;
			group->scanIndex=0;
			if(++savedGroupCount>savedGroupAlloced) {
				savedGroupAlloced+=allocstep;
				savedGroup=(struct SavedGroupState_t*)mir_realloc(savedGroup,sizeof(struct SavedGroupState_t)*savedGroupAlloced);
			}
			savedGroup[savedGroupCount-1].groupId=group->groupId;
			savedGroup[savedGroupCount-1].expanded=group->expanded;
			continue;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_CONTACT) {
			if(++savedContactCount>savedContactAlloced) {
				savedContactAlloced+=allocstep;
				savedContact=(struct SavedContactState_t*)mir_realloc(savedContact,sizeof(struct SavedContactState_t)*savedContactAlloced);
			}
			savedContact[savedContactCount-1].hContact=group->contact[group->scanIndex].hContact;
			CopyMemory(savedContact[savedContactCount-1].iExtraImage,group->contact[group->scanIndex].iExtraImage,sizeof(group->contact[group->scanIndex].iExtraImage));
			savedContact[savedContactCount-1].checked=group->contact[group->scanIndex].flags&CONTACTF_CHECKED;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_INFO) {
			if(++savedInfoCount>savedInfoAlloced) {
				savedInfoAlloced+=allocstep;
				savedInfo=(struct SavedInfoState_t*)mir_realloc(savedInfo,sizeof(struct SavedInfoState_t)*savedInfoAlloced);
			}
			if(group->parent==NULL) savedInfo[savedInfoCount-1].parentId=-1;
			else savedInfo[savedInfoCount-1].parentId=group->groupId;
			savedInfo[savedInfoCount-1].contact=group->contact[group->scanIndex];
		}
		group->scanIndex++;
	}

	FreeGroup(&dat->list);
	RebuildEntireList(hwnd,dat);

	group=&dat->list;
	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->contactCount) {
			group=group->parent;
			if(group==NULL) break;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_GROUP) {
			group=group->contact[group->scanIndex].group;
			group->scanIndex=0;
			for(i=0;i<savedGroupCount;i++)
				if(savedGroup[i].groupId==group->groupId) {
					group->expanded=savedGroup[i].expanded;
					break;
				}
			continue;
		}
		else if(group->contact[group->scanIndex].type==CLCIT_CONTACT) {
			for(i=0;i<savedContactCount;i++)
				if(savedContact[i].hContact==group->contact[group->scanIndex].hContact) {
					CopyMemory(group->contact[group->scanIndex].iExtraImage,savedContact[i].iExtraImage,sizeof(group->contact[group->scanIndex].iExtraImage));
					if(savedContact[i].checked) group->contact[group->scanIndex].flags|=CONTACTF_CHECKED;
					break;
				}
		}
		group->scanIndex++;
	}
	if(savedGroup) mir_free(savedGroup);
	if(savedContact) mir_free(savedContact);
	for(i=0;i<savedInfoCount;i++) {
		if(savedInfo[i].parentId==-1) group=&dat->list;
		else {
			if(!FindItem(hwnd,dat,(HANDLE)(savedInfo[i].parentId|HCONTACT_ISGROUP),&contact,NULL,NULL)) continue;
			group=contact->group;
		}
		j=AddInfoItemToGroup(group,savedInfo[i].contact.flags,"");
		group->contact[j]=savedInfo[i].contact;
	}
	if(savedInfo) mir_free(savedInfo);
	RecalculateGroupCheckboxes(hwnd,dat);

	RecalcScrollBar(hwnd,dat);
	nm.hdr.code=CLN_LISTREBUILT;
	nm.hdr.hwndFrom=hwnd;
	nm.hdr.idFrom=GetDlgCtrlID(hwnd);


	//srand(GetTickCount());
	
	tick=GetTickCount()-tick;
	{
	char buf[255];
	//sprintf(buf,"%s %s took %i ms",__FILE__,__LINE__,tick);
	sprintf(buf,"SaveStateAndRebuildList %d \r\n",tick);

	OutputDebugString(buf);
	}	
	ClearRowByIndexCache();
	SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
}