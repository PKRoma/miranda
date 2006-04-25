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
#include "clist.h"
#include "m_metacontacts.h"
#include "commonprototypes.h"
extern void ( *saveAddContactToTree)(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline);


void AddSubcontacts(struct ClcData *dat, struct ClcContact * cont, BOOL showOfflineHereGroup)
{
	int subcount,i,j;
	HANDLE hsub;
	pdisplayNameCacheEntry cacheEntry;
	DWORD style=GetWindowLong(dat->hWnd,GWL_STYLE);
	cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(cont->hContact);
	cont->SubExpanded=(DBGetContactSettingByte(cont->hContact,"CList","Expanded",0) && (DBGetContactSettingByte(NULL,"CLC","MetaExpanding",1)));
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
		if (showOfflineHereGroup||(!(DBGetContactSettingByte(NULL,"CLC","MetaHideOfflineSub",1) && DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT) ) ||
			cacheEntry->status!=ID_STATUS_OFFLINE )
			//&&
			//(!cacheEntry->Hidden || style&CLS_SHOWHIDDEN)
			)

		{
			cont->subcontacts[i].hContact=cacheEntry->hContact;

			cont->subcontacts[i].avatar_pos = AVATAR_POS_DONT_HAVE;
			Cache_GetAvatar(dat, &cont->subcontacts[i]);

			cont->subcontacts[i].iImage=CallService(MS_CLIST_GETCONTACTICON,(WPARAM)cacheEntry->hContact,0);
			memset(cont->subcontacts[i].iExtraImage,0xFF,sizeof(cont->subcontacts[i].iExtraImage));
			cont->subcontacts[i].proto=cacheEntry->szProto;		
			cont->subcontacts[i].type=CLCIT_CONTACT;
			cont->subcontacts[i].flags=0;//CONTACTF_ONLINE;
			cont->subcontacts[i].isSubcontact=i+1;
			cont->subcontacts[i].subcontacts=cont;
			cont->subcontacts[i].image_is_special=FALSE;
			//cont->subcontacts[i].status=cacheEntry->status;
			Cache_GetTimezone(dat, (&cont->subcontacts[i])->hContact);
			Cache_GetText(dat, &cont->subcontacts[i],0);

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
	if (!i && cont->subcontacts != NULL) mir_free(cont->subcontacts);
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
//				Cache_DestroySmileyList(p->subcontacts[i].plSecondLineText);
//				Cache_DestroySmileyList(p->subcontacts[i].plThirdLineText);
//				if (p->subcontacts[i].szSecondLineText)
//					mir_free(p->subcontacts[i].szSecondLineText);
//				if (p->subcontacts[i].szThirdLineText)
//					mir_free(p->subcontacts[i].szThirdLineText);
			}

			mir_free(p->subcontacts);
		}	}

	Cache_DestroySmileyList(p->plText);
	p->plText=NULL;
//	Cache_DestroySmileyList(p->plSecondLineText);
//	p->plSecondLineText=NULL;
//	Cache_DestroySmileyList(p->plThirdLineText);
//	p->plThirdLineText=NULL;
//	if (p->szSecondLineText)
//		mir_free(p->szSecondLineText);
//	if (p->szThirdLineText)
//		mir_free(p->szThirdLineText);

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
//	group->cl.items[i]->szSecondLineText=NULL;
//	group->cl.items[i]->szThirdLineText=NULL;
	group->cl.items[i]->image_is_special=FALSE;
//	group->cl.items[i]->status=cacheEntry->status;

	group->cl.items[i]->iImage=CallService(MS_CLIST_GETCONTACTICON,(WPARAM)hContact,0);
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
	pcli->pfnInvalidateDisplayNameCacheEntry(hContact);	
	Cache_GetTimezone(dat, group->cl.items[i]->hContact);
	Cache_GetText(dat, group->cl.items[i],0);
	ClearRowByIndexCache();
	return group->cl.items[i];
}

void * AddTempGroup(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers)
{
	int i=0;
	int f=0;
	TCHAR * szGroupName;
	DWORD groupFlags;
#ifdef UNICODE
	char *mbuf=u2a((TCHAR *)szName);
#else
	char *mbuf=mir_strdup((char *)szName);
#endif
	if (WildCompare(mbuf,"-@-HIDDEN-GROUP-@-",0))
	{
		mir_free(mbuf);
		return NULL;
	} 
	mir_free(mbuf);
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
extern _inline BOOL IsShowOfflineGroup(struct ClcGroup* group);
void cli_AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline)
{
	struct ClcGroup *group;
	struct ClcContact * cont;
	pdisplayNameCacheEntry cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hContact);
	if(dat->IsMetaContactsEnabled && cacheEntry && cacheEntry->HiddenSubcontact) return;		//contact should not be added
	if(!dat->IsMetaContactsEnabled && cacheEntry && !mir_strcmp(cacheEntry->szProto,"MetaContacts")) return;
	saveAddContactToTree(hwnd,dat,hContact,updateTotalCount,checkHideOffline);
	if (FindItem(hwnd,dat,hContact,&cont,&group,NULL,FALSE))
	{
		if (cont)
		{
			//Add subcontacts
			if (cont && cont->proto)
			{	
				cont->SubAllocated=0;
				if (mir_strcmp(cont->proto,"MetaContacts")==0) 
					AddSubcontacts(dat,cont,IsShowOfflineGroup(group));
			}
			cont->avatar_pos=AVATAR_POS_DONT_HAVE;
			Cache_GetAvatar(dat,cont);
			Cache_GetText(dat,cont,0);
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

	pcli->pfnFreeCacheItem(pcli->pfnGetCacheEntry(hItem)); //TODO should be called in core

	dat->NeedResort=1;
	ClearRowByIndexCache();
}

//TODO move next line to m_clist.h
#define GROUPF_SHOWOFFLINE 0x80   
_inline BOOL IsShowOfflineGroup(struct ClcGroup* group)
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
	BOOL GroupShowOfflineHere=FALSE;
	int tick=GetTickCount();
	KillTimer(hwnd,TIMERID_REBUILDAFTER);
	lockdat;
	ClearRowByIndexCache();
	ImageArray_Clear(&dat->avatar_cache);
	RowHeights_Clear(dat);
	RowHeights_GetMaxRowHeight(dat, hwnd);
  TRACEVAR("Rebuild Entire List %d times\n",++rebuildCounter);
  
	dat->list.expanded=1;
	dat->list.hideOffline=DBGetContactSettingByte(NULL,"CLC","HideOfflineRoot",0);
	dat->list.cl.count = dat->list.cl.limit = 0;
	dat->list.cl.increment = 50;
	dat->NeedResort=1;
	dat->selection=-1;
	dat->HiLightMode=DBGetContactSettingByte(NULL,"CLC","HiLightMode",0);
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
	while(hContact) {
		pdisplayNameCacheEntry cacheEntry=NULL;
		cont=NULL;
		cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hContact);

		if( (cacheEntry->szProto||style&CLS_SHOWHIDDEN) &&
			(
			 (dat->IsMetaContactsEnabled||mir_strcmp(cacheEntry->szProto,"MetaContacts"))
			 &&(style&CLS_SHOWHIDDEN || (!cacheEntry->Hidden && !cacheEntry->isUnknown)) 
			 &&(!cacheEntry->HiddenSubcontact || !dat->IsMetaContactsEnabled)
			)
		  )
		{

			if(lstrlen(cacheEntry->szGroup)==0)
				group=&dat->list;
			else {
				group=cli_AddGroup(hwnd,dat,cacheEntry->szGroup,(DWORD)-1,0,0);
			}
			if(group!=NULL) 
			{
				if (cacheEntry->status==ID_STATUS_OFFLINE)
					if (DBGetContactSettingByte(NULL,"CList","PlaceOfflineToRoot",0))
						group=&dat->list;
				group->totalMembers++;

				if(!(style&CLS_NOHIDEOFFLINE) && (style&CLS_HIDEOFFLINE || group->hideOffline)) 
				{
					if(cacheEntry->szProto==NULL) {
						if(!pcli->pfnIsHiddenMode(dat,ID_STATUS_OFFLINE)||cacheEntry->noHiddenOffline || IsShowOfflineGroup(group))
							cont=AddContactToGroup(dat,group,cacheEntry);
					}
					else
						if(!pcli->pfnIsHiddenMode(dat,cacheEntry->status)||cacheEntry->noHiddenOffline || IsShowOfflineGroup(group))
							cont=AddContactToGroup(dat,group,cacheEntry);
				}
				else cont=AddContactToGroup(dat,group,cacheEntry);
			}
		}
		if (cont)	
		{	
			cont->SubAllocated=0;
			if (cont->proto && strcmp(cont->proto,"MetaContacts")==0)
				AddSubcontacts(dat,cont,IsShowOfflineGroup(group));
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
	ulockdat;

	pcli->pfnSortCLC(hwnd,dat,0);

}

void cli_SortCLC( HWND hwnd, struct ClcData *dat, int useInsertionSort )
{
	lockdat;
	saveSortCLC(hwnd,dat,useInsertionSort);
	ulockdat;
}

int GetNewSelection(struct ClcGroup *group, int selection, int direction)
{
	int lastcount=0, count=0;//group->cl.count;
	struct ClcGroup *topgroup=group;
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
extern int StoreAllContactData(struct ClcData *dat);
extern int RestoreAllContactData(struct ClcData *dat);
void cli_SaveStateAndRebuildList(HWND hwnd, struct ClcData *dat)
{
	
	lockdat;
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
						SIZEOF(group->cl.items[group->scanIndex]->iExtraImage));
					if (savedContact[i].checked)
						group->cl.items[group->scanIndex]->flags |= CONTACTF_CHECKED;
					break;
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
			if (!pcli->pfnFindItem(hwnd, dat, (HANDLE) (savedInfo[i].parentId | HCONTACT_ISGROUP), &contact, NULL, NULL))
				continue;
			group = contact->group;
		}
		j = pcli->pfnAddInfoItemToGroup(group, savedInfo[i].contact.flags, _T(""));
		*group->cl.items[j] = savedInfo[i].contact;
	}
	if (savedInfo)
		mir_free(savedInfo);
	RestoreAllContactData(dat);
	LOCK_RECALC_SCROLLBAR=FALSE;
	pcli->pfnRecalculateGroupCheckboxes(hwnd, dat);

	pcli->pfnRecalcScrollBar(hwnd, dat);
	nm.hdr.code = CLN_LISTREBUILT;
	nm.hdr.hwndFrom = hwnd;
	nm.hdr.idFrom = GetDlgCtrlID(hwnd);
	SendMessage(GetParent(hwnd), WM_NOTIFY, 0, (LPARAM) & nm);
	}
	ulockdat;
}


struct ClcContact* cliCreateClcContact( void )
{
	return (struct ClcContact*)mir_calloc(1, sizeof( struct ClcContact ) );
}

ClcCacheEntryBase* cliCreateCacheItem( HANDLE hContact )
{
	pdisplayNameCacheEntry p = (pdisplayNameCacheEntry)mir_calloc( 1, sizeof( displayNameCacheEntry ));
	
	if ( p )
	{
		memset(p,0,sizeof( displayNameCacheEntry ));
		p->hContact = hContact;
		InvalidateDNCEbyPointer(hContact,p,0); //TODO should be in core
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
	//if (IsBadWritePtr((void*)hContact,sizeof(displayNameCacheEntry)))
		p = (pdisplayNameCacheEntry) pcli->pfnGetCacheEntry(hContact);
	//else 
	//	p=(pdisplayNameCacheEntry)hContact; //handle give us incorrect hContact on GetCacheEntry;
	if (p)
		InvalidateDNCEbyPointer(hContact,p,0);
	return;
}

char* cli_GetGroupCountsText(struct ClcData *dat, struct ClcContact *contact)
{
	char * res;
	lockdat;
	res=saveGetGroupCountsText(dat, contact);
	ulockdat;
	return res;
}
