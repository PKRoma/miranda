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
#include "m_metacontacts.h"

extern int ( *saveAddItemToGroup )( struct ClcGroup *group, int iAboveItem );
extern int ( *saveAddInfoItemToGroup )(struct ClcGroup *group,int flags,const TCHAR *pszText);

extern void ( *saveDeleteItemFromTree )(HWND hwnd, HANDLE hItem);
extern void ( *saveFreeContact )( struct ClcContact* );
extern void ( *saveFreeGroup )( struct ClcGroup* );

extern struct ClcGroup* ( *saveAddGroup )(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers);

//routines for managing adding/removal of items in the list, including sorting
extern BOOL InvalidateRectZ(HWND hWnd, CONST RECT* lpRect,BOOL bErase );
extern int CompareContacts(WPARAM wParam,LPARAM lParam);
extern void ClearClcContactCache(struct ClcData *dat,HANDLE hContact);
extern BOOL FillRect255Alpha(HDC memdc,RECT *fr);
extern BOOL LOCK_IMAGE_UPDATING;
int lastGroupId=0;

void AddSubcontacts(struct ClcData *dat, struct ClcContact * cont)
{
	int subcount,i,j;
	HANDLE hsub;
	pdisplayNameCacheEntry cacheEntry;
	cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(cont->hContact);
	TRACE("Proceed AddSubcontacts\r\n");
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
		if (!(DBGetContactSettingByte(NULL,"CLC","MetaHideOfflineSub",1) && DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT) ) ||
			cacheEntry->status!=ID_STATUS_OFFLINE )
		{
			cont->subcontacts[i].hContact=cacheEntry->hContact;

			cont->subcontacts[i].avatar_pos = AVATAR_POS_DONT_HAVE;
			Cache_GetAvatar(dat, &cont->subcontacts[i]);

			cont->subcontacts[i].iImage=CallService(MS_CLIST_GETCONTACTICON,(WPARAM)cacheEntry->hContact,0);
			memset(cont->subcontacts[i].iExtraImage,0xFF,sizeof(cont->subcontacts[i].iExtraImage));
			cont->subcontacts[i].proto=cacheEntry->szProto;		

			//lstrcpynA(cont->subcontacts[i]->szText,cacheEntry->name,sizeof(cont->subcontacts[i]->szText));
			//Cache_GetText(dat, &cont->subcontacts[i]);
			cont->subcontacts[i].type=CLCIT_CONTACT;
			cont->subcontacts[i].flags=0;//CONTACTF_ONLINE;
			cont->subcontacts[i].isSubcontact=i+1;
			cont->subcontacts[i].subcontacts=cont;
			cont->subcontacts[i].image_is_special=FALSE;
			cont->subcontacts[i].status=cacheEntry->status;

			//lstrcpyn(cont->subcontacts[i]->szText,cacheEntry->name,sizeof(cont->subcontacts[i]->szText));
			Cache_GetTimezone(dat, &cont->subcontacts[i]);
			Cache_GetText(dat, &cont->subcontacts[i]);

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

int AddItemToGroup(struct ClcGroup *group,int iAboveItem)
{
	if ( group == NULL ) return 0;

	iAboveItem = saveAddItemToGroup( group, iAboveItem );
	ClearRowByIndexCache();
	return iAboveItem;
}

struct ClcGroup *AddGroup(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers)
{
	struct ClcGroup* result;
	ClearRowByIndexCache();	

	if (GetWindowLong(hwnd,GWL_STYLE)&CLS_USEGROUPS)                            //groups is using
		if (ServiceExists(MS_MC_GETDEFAULTCONTACT))                             //metacontacts dll is loaded  
			if (DBGetContactSettingByte(NULL,"MetaContacts","Enabled",1));      //and enabled
	{
		TCHAR* HiddenGroup=DBGetStringT(NULL,"MetaContacts","HiddenGroupName");   //TODO: UNICODE + Hidden groupname
		// if (!HiddenGroup) strdup("MetaContacts Hidden Group"); 
		if (szName && HiddenGroup)
			if (!lstrcmp(HiddenGroup,szName))                                    //group is metahidden
			{   
				if (HiddenGroup) mir_free(HiddenGroup);
				return NULL;
			}
			if (HiddenGroup) mir_free(HiddenGroup);
	}

	result = saveAddGroup( hwnd, dat, szName, flags, groupId, calcTotalMembers);
	ClearRowByIndexCache();
	return result;
}

void FreeContact(struct ClcContact *p)
{
	if ( p->SubAllocated) {
		if ( p->subcontacts && !p->isSubcontact) {
			int i;
			for ( i = 0 ; i < p->SubAllocated ; i++ ) {
				Cache_DestroySmileyList(p->subcontacts[i].plText);
				Cache_DestroySmileyList(p->subcontacts[i].plSecondLineText);
				Cache_DestroySmileyList(p->subcontacts[i].plThirdLineText);
				if (p->subcontacts[i].szSecondLineText)
					mir_free(p->subcontacts[i].szSecondLineText);
				if (p->subcontacts[i].szThirdLineText)
					mir_free(p->subcontacts[i].szThirdLineText);
			}

			mir_free(p->subcontacts);
	}	}

	Cache_DestroySmileyList(p->plText);
	Cache_DestroySmileyList(p->plSecondLineText);
	Cache_DestroySmileyList(p->plThirdLineText);
	if (p->szSecondLineText)
		mir_free(p->szSecondLineText);
	if (p->szThirdLineText)
		mir_free(p->szThirdLineText);

	saveFreeContact( p );
}

void FreeGroup( struct ClcGroup* group )
{
	saveFreeGroup( group );
	ClearRowByIndexCache();
}

int AddInfoItemToGroup(struct ClcGroup *group,int flags,const TCHAR *pszText)
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
	//	DBVARIANT dbv;
	if (cacheEntry==NULL) return NULL;
	if (group==NULL) return NULL;
	if (dat==NULL) return NULL;
	hContact=cacheEntry->hContact;
	//ClearClcContactCache(hContact);

	dat->NeedResort=1;
	for(i=group->cl.count-1;i>=0;i--)
		if(group->cl.items[i]->type!=CLCIT_INFO || !(group->cl.items[i]->flags&CLCIIF_BELOWCONTACTS)) break;
	i=AddItemToGroup(group,i+1);
	group->cl.items[i]->type=CLCIT_CONTACT;
	group->cl.items[i]->SubAllocated=0;
	group->cl.items[i]->isSubcontact=0;
	group->cl.items[i]->subcontacts=NULL;
	group->cl.items[i]->szText[0]=0;
	group->cl.items[i]->szSecondLineText=NULL;
	group->cl.items[i]->szThirdLineText=NULL;
	group->cl.items[i]->image_is_special=FALSE;
	group->cl.items[i]->status=cacheEntry->status;

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
	if (idleMode) group->cl.items[i]->flags|=CONTACTF_IDLE;

	//lstrcpynA(group->cl.items[i]->szText,cacheEntry->name,sizeof(group->cl.items[i]->szText));
	group->cl.items[i]->proto = szProto;

	group->cl.items[i]->timezone = (DWORD)DBGetContactSettingByte(hContact,"UserInfo","Timezone", DBGetContactSettingByte(hContact, szProto,"Timezone",-1));
	if (group->cl.items[i]->timezone != -1)
	{
		int contact_gmt_diff = group->cl.items[i]->timezone;
		contact_gmt_diff = contact_gmt_diff > 128 ? 256 - contact_gmt_diff : 0 - contact_gmt_diff;
		contact_gmt_diff *= 60*60/2;

		// Only in case of same timezone, ignore DST
		if (contact_gmt_diff == dat->local_gmt_diff)
			group->cl.items[i]->timediff = 0;
		else
			group->cl.items[i]->timediff = (int)dat->local_gmt_diff_dst - contact_gmt_diff;
	}

	Cache_GetTimezone(dat, group->cl.items[i]);
	Cache_GetText(dat, group->cl.items[i]);

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
	if (WildCompare(mbuf,"* Hidden Group",0))
	{
		mir_free(mbuf);
		if(ServiceExists(MS_MC_ADDTOMETA)) return NULL;
		else return(&dat->list);
	} 
	mir_free(mbuf);
	for(i=1;;i++) 
	{
		szGroupName = pcli->pfnGetGroupName(i,&groupFlags);
		if(szGroupName==NULL) break;
		if (!MyStrCmpiT(szGroupName,szName)) f=1;
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
		res=AddGroup(hwnd,dat,szName,groupFlags,i,0);
		return res;
	}
	return NULL;
}
void AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline)
{
	struct ClcGroup *group;
	struct ClcContact * cont;
	pdisplayNameCacheEntry cacheEntry;
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);
	WORD status;
	char *szProto;

	if (FindItem(hwnd,dat,hContact,NULL,NULL,NULL,FALSE)==1){return;};	
	cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hContact);
	if (cacheEntry==NULL) return;
	if (dat->IsMetaContactsEnabled && cacheEntry->HiddenSubcontact) return;   ///-----
	szProto=cacheEntry->szProto;

	dat->NeedResort=1;
	ClearRowByIndexCache();
	ClearClcContactCache(dat,hContact);

	if(style&CLS_NOHIDEOFFLINE) checkHideOffline=0;
	if(checkHideOffline) {
		if(szProto==NULL) status=ID_STATUS_OFFLINE;
		else status=cacheEntry->status;
	}

	if(lstrlen(cacheEntry->szGroup)==0)
		group=&dat->list;
	else {
		group=AddGroup(hwnd,dat,cacheEntry->szGroup,(DWORD)-1,0,0);
		if(group==NULL) {
			int i,len;
			DWORD groupFlags;
			TCHAR *szGroupName;
			if(!(style&CLS_HIDEEMPTYGROUPS)) {
				//	/*mir_free(dbv.pszVal);*/AddTempGroup(hwnd,dat,cacheEntry->szGroup,(DWORD)-1,0,0);
				return;
			}
			if(checkHideOffline && pcli->pfnIsHiddenMode(dat,status)) {
				for(i=1;;i++) {
					szGroupName=pcli->pfnGetGroupName(i,&groupFlags); //UNICODE
					if(szGroupName==NULL) {/*mir_free(dbv.pszVal);*/ return;}   //never happens
					if(!lstrcmp(szGroupName,cacheEntry->szGroup)) break;
				}
				if(groupFlags&GROUPF_HIDEOFFLINE) {/*mir_free(dbv.pszVal);*/ return;}
			}
			for(i=1;;i++) {
				szGroupName=pcli->pfnGetGroupName(i,&groupFlags); //UNICODE
				if(szGroupName==NULL) {/*mir_free(dbv.pszVal);*/ return;}   //never happens
				if(!lstrcmp(szGroupName,cacheEntry->szGroup)) break;
				len=lstrlen(szGroupName);
				if(!_tcsncmp(szGroupName,cacheEntry->szGroup,len) && cacheEntry->szGroup[len]=='\\')
					AddGroup(hwnd,dat,szGroupName,groupFlags,i,1);
			}
			group=AddGroup(hwnd,dat,cacheEntry->szGroup,groupFlags,i,1);
		}
		//	mir_free(dbv.pszVal);
	}
	if (cacheEntry->status==ID_STATUS_OFFLINE)
		if (DBGetContactSettingByte(NULL,"CList","PlaceOfflineToRoot",0))
			group=&dat->list;
	if(checkHideOffline) {
		if(pcli->pfnIsHiddenMode(dat,status) && (style&CLS_HIDEOFFLINE || group->hideOffline)) {
			if(updateTotalCount) group->totalMembers++;
			return;
		}
	}
	if(dat->IsMetaContactsEnabled &&  cacheEntry->HiddenSubcontact) return;
	if(!dat->IsMetaContactsEnabled && !MyStrCmp(cacheEntry->szProto,"MetaContacts")) return;
	cont=AddContactToGroup(dat,group,cacheEntry);
	if (cont)	
		if (cont->proto)
		{	
			cont->SubAllocated=0;
			if (MyStrCmp(cont->proto,"MetaContacts")==0)
				AddSubcontacts(dat,cont);
		}
	if(updateTotalCount && group) group->totalMembers++;
	ClearRowByIndexCache();
}

void DeleteItemFromTree(HWND hwnd,HANDLE hItem)
{
	struct ClcData *dat = (struct ClcData *) GetWindowLong(hwnd, 0);
	ClearRowByIndexCache();
	saveDeleteItemFromTree(hwnd, hItem);
	ClearClcContactCache(dat,hItem);
	dat->NeedResort=1;
	ClearRowByIndexCache();
}

void RebuildEntireList(HWND hwnd,struct ClcData *dat)
{
	DWORD style=GetWindowLong(hwnd,GWL_STYLE);
	HANDLE hContact;
	struct ClcContact * cont;
	struct ClcGroup *group;
	int tick=GetTickCount();
	KillTimer(hwnd,TIMERID_REBUILDAFTER);

	//EnterCriticalSection(&(dat->lockitemCS));
	//ShowTracePopup("RebuildEntireList");

#ifdef _DEBUG
	{
		static int num_calls = 0;
		char tmp[128];
		mir_snprintf(tmp, sizeof(tmp), "*********************   RebuildEntireList (%d)\r\n", num_calls);
		num_calls++;
		TRACE(tmp);
	}
#endif 

	ClearRowByIndexCache();
	ClearClcContactCache(dat,INVALID_HANDLE_VALUE);
	ImageArray_Clear(&dat->avatar_cache);
	RowHeights_Clear(dat);
	RowHeights_GetMaxRowHeight(dat, hwnd);

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
			AddGroup(hwnd,dat,szGroupName,groupFlags,i,0);
		}
		lastGroupId=i;

	}

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact) {
		pdisplayNameCacheEntry cacheEntry;
		cont=NULL;
		cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hContact);
		ClearClcContactCache(dat,hContact);

		if((dat->IsMetaContactsEnabled||MyStrCmp(cacheEntry->szProto,"MetaContacts"))&&(style&CLS_SHOWHIDDEN || !cacheEntry->Hidden) && (!cacheEntry->HiddenSubcontact || !dat->IsMetaContactsEnabled )) {
			if(lstrlen(cacheEntry->szGroup)==0)
				group=&dat->list;
			else {
				group=AddGroup(hwnd,dat,cacheEntry->szGroup,(DWORD)-1,0,0);
			}
			if(group!=NULL) {
				if (cacheEntry->status==ID_STATUS_OFFLINE)
					if (DBGetContactSettingByte(NULL,"CList","PlaceOfflineToRoot",0))
						group=&dat->list;
				group->totalMembers++;
				if(!(style&CLS_NOHIDEOFFLINE) && (style&CLS_HIDEOFFLINE || group->hideOffline)) {
					//szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
					if(cacheEntry->szProto==NULL) {
						if(!pcli->pfnIsHiddenMode(dat,ID_STATUS_OFFLINE)||cacheEntry->noHiddenOffline)
							cont=AddContactToGroup(dat,group,cacheEntry);
					}
					else
						if(!pcli->pfnIsHiddenMode(dat,cacheEntry->status)||cacheEntry->noHiddenOffline)
							cont=AddContactToGroup(dat,group,cacheEntry);
				}
				else cont=AddContactToGroup(dat,group,cacheEntry);
			}
		}
		if (cont)	
		{	
			cont->SubAllocated=0;
			if (cont->proto && strcmp(cont->proto,"MetaContacts")==0)
				AddSubcontacts(dat,cont);
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

	SortCLC(hwnd,dat,0);
	// LOCK_IMAGE_UPDATING=0;
	//LeaveCriticalSection(&(dat->lockitemCS));
#ifdef _DEBUG
	tick=GetTickCount()-tick;
	{
		char buf[255];
		//sprintf(buf,"%s %s took %i ms",__FILE__,__LINE__,tick);
		sprintf(buf,"RebuildEntireList %d \r\n",tick);

		TRACE(buf);
		DBWriteContactSettingDword((HANDLE)0,"CLUI","PF:Last RebuildEntireList Time:",tick);
	}	
#endif
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
		/*		if ((group->cl.items[group->scanIndex]->type==CLCIT_CONTACT) && (group->cl.items[group->scanIndex].flags & CONTACTF_STATUSMSG)) {
		count++;
		}
		*/
		if (!direction) {
			if (count>selection) return lastcount;
		}
		if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP && (group->cl.items[group->scanIndex]->group->expanded)) {
			group=group->cl.items[group->scanIndex]->group;
			group->scanIndex=0;
			//	count+=group->cl.count;
			continue;
		}
		group->scanIndex++;
	}
	return lastcount;
}

static int __cdecl GroupSortProc(const struct ClcContact *contact1,const struct ClcContact *contact2)
{
	return lstrcmpi(contact1->szText,contact2->szText);

}

static int __cdecl ContactSortProc(const struct ClcContact **contact1,const struct ClcContact **contact2)
{
	int result = CompareContacts((WPARAM)contact1[0]->hContact,(LPARAM)contact2[0]->hContact);
	if ( result )
		return result;
	
	//nothing to distinguish them, so make sure they stay in the same order
	return (int)contact2[0]->hContact - (int)contact1[0]->hContact;
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

static void SortGroup(struct ClcData *dat,struct ClcGroup *group,int useInsertionSort)
{
	int i, sortCount;

	for(i=group->cl.count-1;i>=0;i--) {
		if(group->cl.items[i]->type==CLCIT_DIVIDER) {
			group->cl.count--;
			memmove(&group->cl.items[i],&group->cl.items[i+1],sizeof(void*)*(group->cl.count-i));
		}
	}
	for(i=0;i<group->cl.count;i++)
		if(group->cl.items[i]->type!=CLCIT_INFO) break;
	if(i>group->cl.count-2) return;
	if(group->cl.items[i]->type==CLCIT_GROUP) {
		if(dat->exStyle&CLS_EX_SORTGROUPSALPHA) {
			for(sortCount=0;i+sortCount<group->cl.count;sortCount++)
				if(group->cl.items[i+sortCount]->type!=CLCIT_GROUP) break;
			qsort(group->cl.items+i,sortCount,sizeof(void*),GroupSortProc);
			i=i+sortCount;
		}
		for(;i<group->cl.count;i++)
			if(group->cl.items[i]->type==CLCIT_CONTACT) break;
		if(group->cl.count-i<2) return;
	}
	for(sortCount=0;i+sortCount<group->cl.count;sortCount++)
		if(group->cl.items[i+sortCount]->type!=CLCIT_CONTACT) break;
	if(useInsertionSort) InsertionSort(group->cl.items+i,sortCount,ContactSortProc);
	else qsort(group->cl.items+i,sortCount,sizeof(void*),ContactSortProc);
	if(dat->exStyle&CLS_EX_DIVIDERONOFF) {
		int prevContactOnline=0;
		for(i=0;i<group->cl.count;i++) 
		{
			if(group->cl.items[i]->type!=CLCIT_CONTACT && group->cl.items[i]->type!=CLCIT_GROUP) continue;
			if ((group->cl.items[i]->type==CLCIT_GROUP) &&  DBGetContactSettingByte(NULL,"CList","PlaceOfflineToRoot",0)) prevContactOnline=1;
			if (group->cl.items[i]->type==CLCIT_CONTACT)
				if(group->cl.items[i]->flags&CONTACTF_ONLINE) prevContactOnline=1;
				else 
				{
					if(prevContactOnline) 
					{
						i=AddItemToGroup(group,i);
						group->cl.items[i]->type=CLCIT_DIVIDER;
						lstrcpyn(group->cl.items[i]->szText,TranslateT("Offline"),sizeof(group->cl.items[i]->szText));
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
	if (dat->NeedResort==1 &&1) {
		if(GetRowByIndex(dat,dat->selection,&selcontact,NULL)==-1) hSelItem=NULL;
		else hSelItem=ContactToHItem(selcontact);
		group->scanIndex=0;

		SortGroup(dat,group,useInsertionSort);

		for(;;) {
			if(group->scanIndex==group->cl.count) {
				group=group->parent;
				if(group==NULL) break;
			}
			else if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP) {
				group=group->cl.items[group->scanIndex]->group;
				group->scanIndex=0;
				SortGroup(dat,group,useInsertionSort);
				continue;
			}
			group->scanIndex++;
		}

		ClearClcContactCache(dat,INVALID_HANDLE_VALUE);

		if(hSelItem)
			if(FindItem(hwnd,dat,hSelItem,&selcontact,&selgroup,NULL,FALSE))
				dat->selection=GetRowsPriorTo(&dat->list,selgroup,selcontact-selgroup->cl.items[0]);


		RecalcScrollBar(hwnd,dat);
		ClearRowByIndexCache();
	}

	InvalidateRectZ(hwnd,NULL,FALSE);
	dat->NeedResort=0;
	// LOCK_IMAGE_UPDATING=1;
	// RecalcScrollBar(hwnd,dat);
#ifdef _DEBUG
	tick=GetTickCount()-tick;
	{
		char buf[255];
		//sprintf(buf,"%s %s took %i ms",__FILE__,__LINE__,tick);
		if (tick>5) 
		{
			sprintf(buf,"SortCLC %d \r\n",tick);
			TRACE(buf);
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

	TRACE("SaveStateAndRebuildList\n");

	pcli->pfnHideInfoTip(hwnd,dat);
	KillTimer(hwnd,TIMERID_INFOTIP);
	KillTimer(hwnd,TIMERID_REBUILDAFTER);
	KillTimer(hwnd,TIMERID_RENAME);
	pcli->pfnEndRename(hwnd,dat,1);

	group=&dat->list;
	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->cl.count) {
			group=group->parent;
			if(group==NULL) break;
		}
		else if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP) {
			group=group->cl.items[group->scanIndex]->group;
			group->scanIndex=0;
			if(++savedGroupCount>savedGroupAlloced) {
				savedGroupAlloced+=allocstep;
				savedGroup=(struct SavedGroupState_t*)mir_realloc(savedGroup,sizeof(struct SavedGroupState_t)*savedGroupAlloced);
			}
			savedGroup[savedGroupCount-1].groupId=group->groupId;
			savedGroup[savedGroupCount-1].expanded=group->expanded;
			continue;
		}
		else if(group->cl.items[group->scanIndex]->type==CLCIT_CONTACT) {			
			if(++savedContactCount>savedContactAlloced) {
				savedContactAlloced+=allocstep;
				savedContact=(struct SavedContactState_t*)mir_realloc(savedContact,sizeof(struct SavedContactState_t)*savedContactAlloced);
			}
			savedContact[savedContactCount-1].hContact=group->cl.items[group->scanIndex]->hContact;
			CopyMemory(savedContact[savedContactCount-1].iExtraImage,group->cl.items[group->scanIndex]->iExtraImage,sizeof(group->cl.items[group->scanIndex]->iExtraImage));
			savedContact[savedContactCount-1].checked=group->cl.items[group->scanIndex]->flags&CONTACTF_CHECKED;
			if (group->cl.items[group->scanIndex]->SubAllocated>0)
			{
				int l;
				for (l=0; l<group->cl.items[group->scanIndex]->SubAllocated; l++)
				{
					if(++savedContactCount>savedContactAlloced) {
						savedContactAlloced+=allocstep;
						savedContact=(struct SavedContactState_t*)mir_realloc(savedContact,sizeof(struct SavedContactState_t)*savedContactAlloced);
					}
					savedContact[savedContactCount-1].hContact=group->cl.items[group->scanIndex]->subcontacts[l].hContact;
					CopyMemory(savedContact[savedContactCount-1].iExtraImage ,group->cl.items[group->scanIndex]->subcontacts[l].iExtraImage,sizeof(group->cl.items[group->scanIndex]->iExtraImage));
					savedContact[savedContactCount-1].checked=group->cl.items[group->scanIndex]->subcontacts[l].flags&CONTACTF_CHECKED;


				}
			}

		}
		else if(group->cl.items[group->scanIndex]->type==CLCIT_INFO) {
			if(++savedInfoCount>savedInfoAlloced) {
				savedInfoAlloced+=allocstep;
				savedInfo=(struct SavedInfoState_t*)mir_realloc(savedInfo,sizeof(struct SavedInfoState_t)*savedInfoAlloced);
			}
			if(group->parent==NULL) savedInfo[savedInfoCount-1].parentId=-1;
			else savedInfo[savedInfoCount-1].parentId=group->groupId;
			savedInfo[savedInfoCount-1].contact = *group->cl.items[group->scanIndex];
			{
				TCHAR * name=NULL;
				if(savedInfo[savedInfoCount-1].contact.szText)
				{
					name=mir_strdupT(savedInfo[savedInfoCount-1].contact.szText);
					lstrcpyn(savedInfo[savedInfoCount-1].contact.szText,name,SIZEOF(savedInfo[savedInfoCount-1].contact.szText));
					group->cl.items[group->scanIndex]->szText[0]=0;
				}
			}
		}
		group->scanIndex++;
	}

	FreeGroup(&dat->list);
	RebuildEntireList(hwnd,dat);

	group=&dat->list;
	group->scanIndex=0;
	for(;;) {
		if(group->scanIndex==group->cl.count) {
			group=group->parent;
			if(group==NULL) break;
		}
		else if(group->cl.items[group->scanIndex]->type==CLCIT_GROUP) {
			group=group->cl.items[group->scanIndex]->group;
			group->scanIndex=0;
			for(i=0;i<savedGroupCount;i++)
				if(savedGroup[i].groupId == group->groupId) {
					group->expanded=savedGroup[i].expanded;
					break;
				}
				continue;
		}
		else if(group->cl.items[group->scanIndex]->type==CLCIT_CONTACT) {
			for(i=0;i<savedContactCount;i++)
				if(savedContact[i].hContact == group->cl.items[group->scanIndex]->hContact) {
					CopyMemory(group->cl.items[group->scanIndex]->iExtraImage,savedContact[i].iExtraImage,sizeof(group->cl.items[group->scanIndex]->iExtraImage));
					if(savedContact[i].checked) group->cl.items[group->scanIndex]->flags|=CONTACTF_CHECKED;
					break;	
				}
				if (group->cl.items[group->scanIndex]->SubAllocated>0)
				{
					int l;
					for (l=0; l<group->cl.items[group->scanIndex]->SubAllocated; l++)
						for(i=0;i<savedContactCount;i++)
							if(savedContact[i].hContact==group->cl.items[group->scanIndex]->subcontacts[l].hContact) {

								CopyMemory(group->cl.items[group->scanIndex]->subcontacts[l].iExtraImage,savedContact[i].iExtraImage,sizeof(group->cl.items[group->scanIndex]->iExtraImage));
								if(savedContact[i].checked) group->cl.items[group->scanIndex]->subcontacts[l].flags|=CONTACTF_CHECKED;
								group->cl.items[group->scanIndex]->subcontacts[l].subcontacts = group->cl.items[group->scanIndex];
								break;	
							}	
				}
		}
		group->scanIndex++;
	}
	if(savedGroup) mir_free(savedGroup);
	if(savedContact) mir_free(savedContact);
	for(i=0;i<savedInfoCount;i++) {
		if(savedInfo[i].parentId==-1) group=&dat->list;
		else {
			if(!FindItem(hwnd,dat,(HANDLE)(savedInfo[i].parentId|HCONTACT_ISGROUP),&contact,NULL,NULL,TRUE)) continue;
			group=contact->group;
		}
		j=AddInfoItemToGroup(group,savedInfo[i].contact.flags,NULL);
		*group->cl.items[j] = savedInfo[i].contact;
	}
	if(savedInfo) mir_free(savedInfo);
	pcli->pfnRecalculateGroupCheckboxes(hwnd,dat);

	RecalcScrollBar(hwnd,dat);
	nm.hdr.code=CLN_LISTREBUILT;
	nm.hdr.hwndFrom=hwnd;
	nm.hdr.idFrom=GetDlgCtrlID(hwnd);


	//srand(GetTickCount());

	tick=GetTickCount()-tick;
#ifdef _DEBUG
	{
		char buf[255];
		sprintf(buf,"SaveStateAndRebuildList %d \r\n",tick);
		TRACE(buf);
	}	
#endif

	ClearRowByIndexCache();
	// SetAllExtraIcons(hwnd,0);
	SendMessage(GetParent(hwnd),WM_NOTIFY,0,(LPARAM)&nm);
}