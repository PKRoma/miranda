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
#include "clc.h"
#include "clist.h"
#include "./m_api/m_metacontacts.h"
#include "commonprototypes.h"



void AddSubcontacts(struct ClcData *dat, struct ClcContact * cont, BOOL showOfflineHereGroup)
{
	int subcount,i,j;
	HANDLE hsub;
	pdisplayNameCacheEntry cacheEntry;
	cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(cont->hContact);
	cont->SubExpanded=(DBGetContactSettingByte(cont->hContact,"CList","Expanded",0) && (DBGetContactSettingByte(NULL,"CLC","MetaExpanding",SETTING_METAEXPANDING_DEFAULT)));
	subcount=(int)CallService(MS_MC_GETNUMCONTACTS,(WPARAM)cont->hContact,0);

	if (subcount <= 0) {
		cont->isSubcontact=0;
		cont->subcontacts=NULL;
		cont->SubAllocated=0;
		return;
	}

	cont->isSubcontact=0;
	cont->subcontacts=(struct ClcContact *) mir_alloc(sizeof(struct ClcContact)*subcount);
	ZeroMemory(cont->subcontacts, sizeof(struct ClcContact)*subcount);
	cont->SubAllocated=subcount;
	i=0;
	for (j=0; j<subcount; j++) {
		hsub=(HANDLE)CallService(MS_MC_GETSUBCONTACT,(WPARAM)cont->hContact,j);
		cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hsub);		
		if (showOfflineHereGroup||(!(DBGetContactSettingByte(NULL,"CLC","MetaHideOfflineSub",SETTING_METAHIDEOFFLINESUB_DEFAULT) && DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT) ) ||
			cacheEntry->status!=ID_STATUS_OFFLINE )
			//&&
			//(!cacheEntry->Hidden || style&CLS_SHOWHIDDEN)
			)

		{
			cont->subcontacts[i].hContact=cacheEntry->hContact;

			cont->subcontacts[i].avatar_pos = AVATAR_POS_DONT_HAVE;
			Cache_GetAvatar(dat, &cont->subcontacts[i]);

			cont->subcontacts[i].iImage=CallService(MS_CLIST_GETCONTACTICON,(WPARAM)cacheEntry->hContact,1);
			memset(cont->subcontacts[i].iExtraImage,0xFF,sizeof(cont->subcontacts[i].iExtraImage));
			memset((void*)cont->subcontacts[i].iWideExtraImage,0xFF,sizeof(cont->subcontacts[i].iWideExtraImage));
			cont->subcontacts[i].proto=cacheEntry->szProto;		
			cont->subcontacts[i].type=CLCIT_CONTACT;
			cont->subcontacts[i].flags=0;//CONTACTF_ONLINE;
			cont->subcontacts[i].isSubcontact=i+1;
            cont->subcontacts[i].lastPaintCounter=0;
			cont->subcontacts[i].subcontacts=cont;
			cont->subcontacts[i].image_is_special=FALSE;
			//cont->subcontacts[i].status=cacheEntry->status;
			Cache_GetTimezone(dat, (&cont->subcontacts[i])->hContact);
			Cache_GetText(dat, &cont->subcontacts[i],1);

			{
				int apparentMode;
				char *szProto;  
				int idleMode;
				szProto=cacheEntry->szProto;
				if(szProto!=NULL&&!pcli->pfnIsHiddenMode(dat,cacheEntry->status))
					cont->subcontacts[i].flags|=CONTACTF_ONLINE;
				apparentMode=szProto!=NULL?cacheEntry->ApparentMode:0;
				apparentMode=szProto!=NULL?cacheEntry->ApparentMode:0;
				if(apparentMode==ID_STATUS_OFFLINE)	cont->subcontacts[i].flags|=CONTACTF_INVISTO;
				else if(apparentMode==ID_STATUS_ONLINE) cont->subcontacts[i].flags|=CONTACTF_VISTO;
				else if(apparentMode) cont->subcontacts[i].flags|=CONTACTF_VISTO|CONTACTF_INVISTO;
				if(cacheEntry->NotOnList) cont->subcontacts[i].flags|=CONTACTF_NOTONLIST;
				idleMode=szProto!=NULL?cacheEntry->IdleTS:0;
				if (idleMode) cont->subcontacts[i].flags|=CONTACTF_IDLE;
            }
			i++;
		}	}

	cont->SubAllocated=i;
	if (!i && cont->subcontacts != NULL) mir_free_and_nill(cont->subcontacts);
}

int cli_AddItemToGroup(struct ClcGroup *group,int iAboveItem)
{
	if ( group == NULL ) return 0;

	iAboveItem = saveAddItemToGroup( group, iAboveItem );
	ClearRowByIndexCache();
	return iAboveItem;
}

struct ClcGroup *cli_AddGroup(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers)
{
	struct ClcGroup* result;
	ClearRowByIndexCache();	
	if (!dat->force_in_dialog && !(GetWindowLong(hwnd, GWL_STYLE) & CLS_SHOWHIDDEN))
		if (!lstrcmp(_T("-@-HIDDEN-GROUP-@-"),szName))        //group is hidden
		{   	
			ClearRowByIndexCache();
			return NULL;
		}
		result = saveAddGroup( hwnd, dat, szName, flags, groupId, calcTotalMembers);
		ClearRowByIndexCache();
		return result;
}

void cli_FreeContact(struct ClcContact *p)
{
	if ( p->SubAllocated) {
		if ( p->subcontacts && !p->isSubcontact) {
			int i;
			for ( i = 0 ; i < p->SubAllocated ; i++ ) {
				Cache_DestroySmileyList(p->subcontacts[i].plText);
				if ( p->subcontacts[i].avatar_pos==AVATAR_POS_ANIMATED )
					AniAva_RemoveAvatar( p->subcontacts[i].hContact );
					p->subcontacts[i].avatar_pos=AVATAR_POS_DONT_HAVE;
			}
			mir_free_and_nill(p->subcontacts);
	}	}

	Cache_DestroySmileyList(p->plText);
	p->plText=NULL;
	if ( p->avatar_pos==AVATAR_POS_ANIMATED )
		AniAva_RemoveAvatar( p->hContact );
	p->avatar_pos=AVATAR_POS_DONT_HAVE;
	saveFreeContact( p );
}

void cli_FreeGroup( struct ClcGroup* group )
{
	saveFreeGroup( group );
	ClearRowByIndexCache();
}

int cli_AddInfoItemToGroup(struct ClcGroup *group,int flags,const TCHAR *pszText)
{
	int i = saveAddInfoItemToGroup( group, flags, pszText );
	ClearRowByIndexCache();
	return i;
}

static struct ClcContact * AddContactToGroup(struct ClcData *dat,struct ClcGroup *group,pdisplayNameCacheEntry cacheEntry)
{
	char *szProto;
	WORD apparentMode;
	DWORD idleMode;
	HANDLE hContact;
	int i;
	if (cacheEntry==NULL) return NULL;
	if (group==NULL) return NULL;
	if (dat==NULL) return NULL;
	hContact=cacheEntry->hContact;
	//ClearClcContactCache(hContact);

	dat->NeedResort=1;
	for(i=group->cl.count-1;i>=0;i--)
		if(group->cl.items[i]->type!=CLCIT_INFO || !(group->cl.items[i]->flags&CLCIIF_BELOWCONTACTS)) break;
	i=cli_AddItemToGroup(group,i+1);
	group->cl.items[i]->type=CLCIT_CONTACT;
	group->cl.items[i]->SubAllocated=0;
	group->cl.items[i]->isSubcontact=0;
	group->cl.items[i]->subcontacts=NULL;
	group->cl.items[i]->szText[0]=0;
    group->cl.items[i]->lastPaintCounter=0;
//	group->cl.items[i]->szSecondLineText=NULL;
//	group->cl.items[i]->szThirdLineText=NULL;
	group->cl.items[i]->image_is_special=FALSE;
//	group->cl.items[i]->status=cacheEntry->status;

	group->cl.items[i]->iImage=CallService(MS_CLIST_GETCONTACTICON,(WPARAM)hContact,1);
	cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hContact);
	group->cl.items[i]->hContact=hContact;

	group->cl.items[i]->avatar_pos = AVATAR_POS_DONT_HAVE;
	Cache_GetAvatar(dat, group->cl.items[i]);

	szProto=cacheEntry->szProto;
	if(szProto!=NULL&&!pcli->pfnIsHiddenMode(dat,cacheEntry->status))
		group->cl.items[i]->flags |= CONTACTF_ONLINE;
	apparentMode=szProto!=NULL?cacheEntry->ApparentMode:0;
	if(apparentMode==ID_STATUS_OFFLINE)	group->cl.items[i]->flags|=CONTACTF_INVISTO;
	else if(apparentMode==ID_STATUS_ONLINE) group->cl.items[i]->flags|=CONTACTF_VISTO;
	else if(apparentMode) group->cl.items[i]->flags|=CONTACTF_VISTO|CONTACTF_INVISTO;
	if(cacheEntry->NotOnList) group->cl.items[i]->flags|=CONTACTF_NOTONLIST;
	idleMode=szProto!=NULL?cacheEntry->IdleTS:0;
	if (idleMode) 
		group->cl.items[i]->flags|=CONTACTF_IDLE;
	group->cl.items[i]->proto = szProto;
//	group->cl.items[i]->timezone = (DWORD)DBGetContactSettingByte(hContact,"UserInfo","Timezone", DBGetContactSettingByte(hContact, szProto,"Timezone",-1));
/*
if (group->cl.items[i]->timezone != -1)
	{
		int contact_gmt_diff = group->cl.items[i]->timezone;
		contact_gmt_diff = contact_gmt_diff > 128 ? 256 - contact_gmt_diff : 0 - contact_gmt_diff;
		contact_gmt_diff *= 60*60/2;

		if (contact_gmt_diff == dat->local_gmt_diff)
			group->cl.items[i]->timediff = 0;
		else
			group->cl.items[i]->timediff = (int)dat->local_gmt_diff_dst - contact_gmt_diff;
	}
*/
                //transports
	pcli->pfnInvalidateDisplayNameCacheEntry(hContact);	
	Cache_GetTimezone(dat, group->cl.items[i]->hContact);
	Cache_GetText(dat, group->cl.items[i],1);
	ClearRowByIndexCache();
    group->cl.items[i]->bContactRate=DBGetContactSettingByte(hContact, "CList", "Rate",0);
	return group->cl.items[i];
}

void * AddTempGroup(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers)
{
	int i=0;
	int f=0;
	TCHAR * szGroupName;
	DWORD groupFlags;
#ifdef UNICODE
	char *mbuf=mir_u2a((TCHAR *)szName);
#else
	char *mbuf=mir_strdup((char *)szName);
#endif
	if (wildcmp(mbuf,"-@-HIDDEN-GROUP-@-",0))
	{
		mir_free_and_nill(mbuf);
		return NULL;
	} 
	mir_free_and_nill(mbuf);
	for(i=1;;i++) 
	{
		szGroupName = pcli->pfnGetGroupName(i,&groupFlags);
		if(szGroupName==NULL) break;
		if (!mir_tstrcmpi(szGroupName,szName)) f=1;
	}
	if (!f)
	{
		char buf[20];
		TCHAR b2[255];
		void * res=NULL;
		_snprintf(buf,sizeof(buf),"%d",(i-1));
		_sntprintf(b2,sizeof(b2),_T("#%s"),szName);
		b2[0]=1|GROUPF_EXPANDED;
		DBWriteContactSettingTString(NULL,"CListGroups",buf,b2);
		pcli->pfnGetGroupName(i,&groupFlags);      
		res=cli_AddGroup(hwnd,dat,szName,groupFlags,i,0);
		return res;
	}
	return NULL;
}

void cli_AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline)
{
	struct ClcGroup *group;
	struct ClcContact * cont;
	pdisplayNameCacheEntry cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hContact);
	if(dat->IsMetaContactsEnabled && cacheEntry && cacheEntry->HiddenSubcontact) return;		//contact should not be added
	if(!dat->IsMetaContactsEnabled && cacheEntry && meta_module && !mir_strcmp(cacheEntry->szProto,meta_module)) return;
	saveAddContactToTree(hwnd,dat,hContact,updateTotalCount,checkHideOffline);
	if (FindItem(hwnd,dat,hContact,&cont,&group,NULL,FALSE))
	{
		if (cont)
		{
			//Add subcontacts
			if (cont && cont->proto)
			{	
				cont->SubAllocated=0;
				if (meta_module && mir_strcmp(cont->proto,meta_module)==0) 
					AddSubcontacts(dat,cont,CLCItems_IsShowOfflineGroup(group));
			}
            cont->lastPaintCounter=0;
			cont->avatar_pos=AVATAR_POS_DONT_HAVE;
			Cache_GetAvatar(dat,cont);
			Cache_GetText(dat,cont,1);
			Cache_GetTimezone(dat,cont->hContact);
		}
	}
	return;
}

void cli_DeleteItemFromTree(HWND hwnd,HANDLE hItem)
{
	struct ClcData *dat = (struct ClcData *) GetWindowLong(hwnd, 0);
	ClearRowByIndexCache();
	saveDeleteItemFromTree(hwnd, hItem);

	//check here contacts are not resorting
	if (hwnd==pcli->hwndContactTree)
		pcli->pfnFreeCacheItem(pcli->pfnGetCacheEntry(hItem)); 
	dat->NeedResort=1;
	ClearRowByIndexCache();
}

//TODO move next line to m_clist.h
#define GROUPF_SHOWOFFLINE 0x80   
__inline BOOL CLCItems_IsShowOfflineGroup(struct ClcGroup* group)
{
	DWORD groupFlags=0;
	if (!group) return FALSE;
	if (group->hideOffline) return FALSE;
	pcli->pfnGetGroupName(group->groupId,&groupFlags);
	return (groupFlags&GROUPF_SHOWOFFLINE)!=0;
}


void cliRebuildEntireList(HWND hwnd,struct ClcData *dat)
{
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);
	HANDLE hContact;
	struct ClcContact * cont;
	struct ClcGroup *group;
    static int rebuildCounter=0;

    BOOL PlaceOfflineToRoot=DBGetContactSettingByte(NULL,"CList","PlaceOfflineToRoot",SETTING_PLACEOFFLINETOROOT_DEFAULT);
	KillTimer(hwnd,TIMERID_REBUILDAFTER);
	
	ClearRowByIndexCache();
	ImageArray_Clear(&dat->avatar_cache);
	RowHeights_Clear(dat);
	RowHeights_GetMaxRowHeight(dat, hwnd);
    TRACEVAR("Rebuild Entire List %d times\n",++rebuildCounter);
  
	dat->list.expanded=1;
	dat->list.hideOffline=DBGetContactSettingByte(NULL,"CLC","HideOfflineRoot",SETTING_HIDEOFFLINEATROOT_DEFAULT) && style&CLS_USEGROUPS;
	dat->list.cl.count = dat->list.cl.limit = 0;
	dat->list.cl.increment = 50;
	dat->NeedResort=1;
	dat->selection=-1;
	dat->HiLightMode=DBGetContactSettingByte(NULL,"CLC","HiLightMode",SETTING_HILIGHTMODE_DEFAULT);
	{
		int i;
		TCHAR *szGroupName;
		DWORD groupFlags;

		for(i=1;;i++) {
			szGroupName=pcli->pfnGetGroupName(i,&groupFlags); //UNICODE
			if(szGroupName==NULL) break;
			cli_AddGroup(hwnd,dat,szGroupName,groupFlags,i,0);
		}
	}

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact) 
    {
		pdisplayNameCacheEntry cacheEntry=NULL;
        int nHiddenStatus;
		cont=NULL;
		cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hContact);
/*
		if( (cacheEntry->szProto || style&CLS_SHOWHIDDEN ) &&
			(
			 (dat->IsMetaContactsEnabled||(meta_module && mir_strcmp(cacheEntry->szProto,meta_module))
			 &&(style&CLS_SHOWHIDDEN || (!cacheEntry->Hidden && !cacheEntry->isUnknown)) 
			 &&(!cacheEntry->HiddenSubcontact || !dat->IsMetaContactsEnabled)
			)
		  )
*/		
        nHiddenStatus=CLVM_GetContactHiddenStatus(hContact, NULL, dat);
		if ( (style&CLS_SHOWHIDDEN && nHiddenStatus!=-1) || !nHiddenStatus)
		{

			if(lstrlen(cacheEntry->szGroup)==0)
				group=&dat->list;
			else {
				group=cli_AddGroup(hwnd,dat,cacheEntry->szGroup,(DWORD)-1,0,0);
			}
			if(group!=NULL) 
			{
				if (cacheEntry->status==ID_STATUS_OFFLINE)
					if (PlaceOfflineToRoot)
						group=&dat->list;
				group->totalMembers++;

				if(!(style&CLS_NOHIDEOFFLINE) && (style&CLS_HIDEOFFLINE || group->hideOffline)) 
				{
					if(cacheEntry->szProto==NULL) {
						if(!pcli->pfnIsHiddenMode(dat,ID_STATUS_OFFLINE)||cacheEntry->noHiddenOffline || CLCItems_IsShowOfflineGroup(group))
							cont=AddContactToGroup(dat,group,cacheEntry);
					}
					else
						if(!pcli->pfnIsHiddenMode(dat,cacheEntry->status)||cacheEntry->noHiddenOffline || CLCItems_IsShowOfflineGroup(group))
							cont=AddContactToGroup(dat,group,cacheEntry);
				}
				else cont=AddContactToGroup(dat,group,cacheEntry);
			}
		}
		if (cont)	
		{	
			cont->SubAllocated=0;
			if (cont->proto && meta_module && strcmp(cont->proto,meta_module)==0)
				AddSubcontacts(dat,cont,CLCItems_IsShowOfflineGroup(group));
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}

	if(style&CLS_HIDEEMPTYGROUPS) {
		group=&dat->list;
		group->scanIndex=0;
		for(;;) {
			if(group->scanIndex==group->cl.count) {
				group=group->parent;
				if(group==NULL) break;
			}
			else if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP) {
				if(group->cl.items[group->scanIndex]->group->cl.count==0) {
					group=pcli->pfnRemoveItemFromGroup(hwnd,group,group->cl.items[group->scanIndex],0);
				}
				else {
					group=group->cl.items[group->scanIndex]->group;
					group->scanIndex=0;
				}
				continue;
			}
			group->scanIndex++;
		}
	}
	

	pcli->pfnSortCLC(hwnd,dat,0);

}

void cli_SortCLC( HWND hwnd, struct ClcData *dat, int useInsertionSort )
{
	HANDLE hSelItem;
	struct ClcContact *selcontact;
	struct ClcGroup *selgroup;
	
	if ( 1 ) {
		if (pcli->pfnGetRowByIndex(dat, dat->selection, &selcontact, NULL) == -1)
			hSelItem = NULL;
		else
			hSelItem = pcli->pfnContactToHItem(selcontact);
	}
	saveSortCLC(hwnd,dat,useInsertionSort);
	if (hSelItem)
		if (pcli->pfnFindItem(hwnd, dat, hSelItem, &selcontact, &selgroup, NULL))
		{
			if (!selcontact->isSubcontact)
				dat->selection =pcli->pfnGetRowsPriorTo(&dat->list, selgroup, li.List_IndexOf((SortedList*)&selgroup->cl,selcontact));
			else
			{  
				int i=selcontact->isSubcontact;
				dat->selection=pcli->pfnGetRowsPriorTo(&dat->list, selgroup, li.List_IndexOf((SortedList*)&selgroup->cl,selcontact->subcontacts));
				if (dat->selection!=-1) dat->selection+=i;
			}
		}
	
}

int GetNewSelection(struct ClcGroup *group, int selection, int direction)
{
	int lastcount=0, count=0;//group->cl.count;
	if (selection<0) {
		return 0;
	}
	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->cl.count) {
			group=group->parent;
			if(group==NULL) break;
			group->scanIndex++;
			continue;
		}
		if (count>=selection) return count;
		lastcount = count;
		count++;
		if (!direction) {
			if (count>selection) return lastcount;
		}
		if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP && (group->cl.items[group->scanIndex]->group->expanded)) {
			group=group->cl.items[group->scanIndex]->group;
			group->scanIndex=0;
			continue;
		}
		group->scanIndex++;
	}
	return lastcount;
}

struct SavedContactState_t {
	HANDLE hContact;
	BYTE iExtraImage[MAXEXTRACOLUMNS];
	WORD iWideExtraImage[MAXEXTRACOLUMNS];
	int checked;

};

struct SavedGroupState_t {
	int groupId,expanded;
};

struct SavedInfoState_t {
	int parentId;
	struct ClcContact contact;
};

BOOL LOCK_RECALC_SCROLLBAR=FALSE;
void cli_SaveStateAndRebuildList(HWND hwnd, struct ClcData *dat)
{
	
	
	LOCK_RECALC_SCROLLBAR=TRUE;
	//saveSaveStateAndRebuildList(hwnd,dat);
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
	
	StoreAllContactData(dat);
	
	pcli->pfnHideInfoTip(hwnd, dat);
	KillTimer(hwnd, TIMERID_INFOTIP);
	KillTimer(hwnd, TIMERID_RENAME);
	pcli->pfnEndRename(hwnd, dat, 1);

	dat->NeedResort = 1;
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
				savedGroup = (struct SavedGroupState_t *) mir_realloc(savedGroup, sizeof(struct SavedGroupState_t) * savedGroupAlloced);
			}
			savedGroup[savedGroupCount - 1].groupId = group->groupId;
			savedGroup[savedGroupCount - 1].expanded = group->expanded;
			continue;
		}
		else if (group->cl.items[group->scanIndex]->type == CLCIT_CONTACT) {
			if (++savedContactCount > savedContactAlloced) {
				savedContactAlloced += 16;
				savedContact = (struct SavedContactState_t *) mir_realloc(savedContact, sizeof(struct SavedContactState_t) * savedContactAlloced);
			}
			savedContact[savedContactCount - 1].hContact = group->cl.items[group->scanIndex]->hContact;
			CopyMemory(savedContact[savedContactCount - 1].iExtraImage, group->cl.items[group->scanIndex]->iExtraImage,
				sizeof(group->cl.items[group->scanIndex]->iExtraImage));
			
			CopyMemory((void*)savedContact[savedContactCount - 1].iWideExtraImage, (void*)group->cl.items[group->scanIndex]->iWideExtraImage,
				sizeof(group->cl.items[group->scanIndex]->iWideExtraImage));
			
			savedContact[savedContactCount - 1].checked = group->cl.items[group->scanIndex]->flags & CONTACTF_CHECKED;
		}
		else if (group->cl.items[group->scanIndex]->type == CLCIT_INFO) {
			if (++savedInfoCount > savedInfoAlloced) {
				savedInfoAlloced += 4;
				savedInfo = (struct SavedInfoState_t *) mir_realloc(savedInfo, sizeof(struct SavedInfoState_t) * savedInfoAlloced);
			}
			if (group->parent == NULL)
				savedInfo[savedInfoCount - 1].parentId = -1;
			else
				savedInfo[savedInfoCount - 1].parentId = group->groupId;
			savedInfo[savedInfoCount - 1].contact = *group->cl.items[group->scanIndex];
		}
		group->scanIndex++;
	}

	pcli->pfnFreeGroup(&dat->list);
	pcli->pfnRebuildEntireList(hwnd, dat);

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
						sizeof(group->cl.items[group->scanIndex]->iExtraImage));
					
					CopyMemory((void*)group->cl.items[group->scanIndex]->iWideExtraImage, (void*)savedContact[i].iWideExtraImage,
						sizeof(group->cl.items[group->scanIndex]->iWideExtraImage));
					
					if (savedContact[i].checked)
						group->cl.items[group->scanIndex]->flags |= CONTACTF_CHECKED;
					break;
				}
		}
		group->scanIndex++;
	}
	if (savedGroup)
		mir_free_and_nill(savedGroup);
	if (savedContact)
		mir_free_and_nill(savedContact);
	for (i = 0; i < savedInfoCount; i++) {
		if (savedInfo[i].parentId == -1)
			group = &dat->list;
		else {
			if (!pcli->pfnFindItem(hwnd, dat, (HANDLE) (savedInfo[i].parentId | HCONTACT_ISGROUP), &contact, NULL, NULL))
				continue;
			group = contact->group;
		}
		j = pcli->pfnAddInfoItemToGroup(group, savedInfo[i].contact.flags, _T(""));
		*group->cl.items[j] = savedInfo[i].contact;
	}
	if (savedInfo)
		mir_free_and_nill(savedInfo);
	RestoreAllContactData(dat);
	LOCK_RECALC_SCROLLBAR=FALSE;
	pcli->pfnRecalculateGroupCheckboxes(hwnd, dat);

	pcli->pfnRecalcScrollBar(hwnd, dat);
	nm.hdr.code = CLN_LISTREBUILT;
	nm.hdr.hwndFrom = hwnd;
	nm.hdr.idFrom = GetDlgCtrlID(hwnd);
	SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) & nm);
	}
	
}


struct ClcContact* cliCreateClcContact( void )
{
	 struct ClcContact* contact=(struct ClcContact*)mir_calloc(sizeof( struct ClcContact ) );
	 memset((void*)contact->iWideExtraImage,0xFF,sizeof(contact->iWideExtraImage));
	 return contact;
}

ClcCacheEntryBase* cliCreateCacheItem( HANDLE hContact )
{
	pdisplayNameCacheEntry p = (pdisplayNameCacheEntry)mir_calloc(sizeof( displayNameCacheEntry ));
	if ( p )
	{
		memset(p,0,sizeof( displayNameCacheEntry ));
		p->hContact = hContact;
		InvalidateDNCEbyPointer(hContact,p,0);
		p->szSecondLineText=NULL;
		p->szThirdLineText=NULL;
		p->plSecondLineText=NULL;
		p->plThirdLineText=NULL;
	}
	return (ClcCacheEntryBase*)p;
}



void cliInvalidateDisplayNameCacheEntry(HANDLE hContact)
{	
	pdisplayNameCacheEntry p;
	p = (pdisplayNameCacheEntry) pcli->pfnGetCacheEntry(hContact);
	if (p) InvalidateDNCEbyPointer(hContact,p,0);
	return;
}

char* cli_GetGroupCountsText(struct ClcData *dat, struct ClcContact *contact)
{
	char * res;
	
	res=saveGetGroupCountsText(dat, contact);
	
	return res;
}

int cliGetGroupContentsCount(struct ClcGroup *group, int visibleOnly)
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
		else if (group->cl.items[group->scanIndex]->type == CLCIT_GROUP && (!(visibleOnly&0x01) || group->cl.items[group->scanIndex]->group->expanded)) {
			group = group->cl.items[group->scanIndex]->group;
			group->scanIndex = 0;
			count += group->cl.count;
			continue;
		}
        else if ((group->cl.items[group->scanIndex]->type == CLCIT_CONTACT) && 
                 (group->cl.items[group->scanIndex]->subcontacts !=NULL)  && 
                 ((group->cl.items[group->scanIndex]->SubExpanded || (!visibleOnly))))
        {
            count+=group->cl.items[group->scanIndex]->SubAllocated;
        }
		group->scanIndex++;
	}
	return count;
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
    PDNCE pdnce=(PDNCE)pcli->pfnGetCacheEntry(hContact);
	BOOL fEmbedded=dat->force_in_dialog;
	// always hide subcontacts (but show them on embedded contact lists)
	
	if(g_CluiData.bMetaAvail && dat != NULL && dat->IsMetaContactsEnabled && meta_module && DBGetContactSettingByte(hContact, meta_module, "IsSubcontact", 0))
		return -1; //subcontact
    if (pdnce && pdnce->isUnknown && !fEmbedded)    
        return 1; //'Unknown Contact'
	if(pdnce && g_CluiData.bFilterEffective && !fEmbedded) 
	{
		if(szProto == NULL)
			szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		// check stickies first (priority), only if we really have stickies defined (CLVM_STICKY_CONTACTS is set).
		if(g_CluiData.bFilterEffective & CLVM_STICKY_CONTACTS) 
        {
			if((dwLocalMask = DBGetContactSettingDword(hContact, CLVM_MODULE, g_CluiData.current_viewmode, 0)) != 0) {
				if(g_CluiData.bFilterEffective & CLVM_FILTER_STICKYSTATUS) 
                {
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
				_sntprintf(szGroupMask, SIZEOF(szGroupMask), _T("%s|"), &dbv.ptszVal[0]);
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
		if(g_CluiData.bFilterEffective & CLVM_FILTER_LASTMSG) 
		{
			DWORD now;
			PDNCE pdnce=(PDNCE)pcli->pfnGetCacheEntry(hContact);
			if (pdnce)
			{
				now = g_CluiData.t_now;
				now -= g_CluiData.lastMsgFilter;
				if(g_CluiData.bFilterEffective & CLVM_FILTER_LASTMSG_OLDERTHAN)
					filterResult = filterResult & (pdnce->dwLastMsgTime < now);
				else if(g_CluiData.bFilterEffective & CLVM_FILTER_LASTMSG_NEWERTHAN)
					filterResult = filterResult & (pdnce->dwLastMsgTime > now);
			}
		}
		return (dbHidden | !filterResult);
	}
	else
		return dbHidden;
}
