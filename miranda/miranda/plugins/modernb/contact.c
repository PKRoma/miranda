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
#include "m_clui.h"
#include "clist.h"

extern HANDLE hContactIconChangedEvent;
extern int GetContactCachedStatus(HANDLE hContact);
extern char *GetContactCachedProtocol(HANDLE hContact);

int sortBy[3], sortNoOfflineBottom;
struct {
	int status,order;
} statusModeOrder[]={
	{ID_STATUS_OFFLINE,500},
	{ID_STATUS_ONLINE,0},
	{ID_STATUS_AWAY,200},
	{ID_STATUS_DND,400},
	{ID_STATUS_NA,450},
	{ID_STATUS_OCCUPIED,100},
	{ID_STATUS_FREECHAT,50},
	{ID_STATUS_INVISIBLE,20},
	{ID_STATUS_ONTHEPHONE,150},
	{ID_STATUS_OUTTOLUNCH,425}};

static int GetContactStatus(HANDLE hContact)
{
	/*
	
	char *szProto;

	szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
	if(szProto==NULL) return ID_STATUS_OFFLINE;
	return DBGetContactSettingWord(hContact,szProto,"Status",ID_STATUS_OFFLINE);
	*/
	return (GetContactCachedStatus(hContact));
}

extern void ( *saveChangeContactIcon)(HANDLE hContact,int iIcon,int add);
void cli_ChangeContactIcon(HANDLE hContact,int iIcon,int add)
{
	pdisplayNameCacheEntry cacheEntry;
	HANDLE hMostMeta=NULL;
	cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry((HANDLE)hContact);
	if (iIcon)
		if (cacheEntry)
			if (!mir_strcmp(cacheEntry->szProto,"MetaContacts"))
				if (!DBGetContactSettingByte(NULL,"CLC","Meta",0))
				{
					int iMetaStatusIcon;
					char * szMetaProto;
					int MetaStatus;
					szMetaProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(UINT)hContact,0);
					MetaStatus=DBGetContactSettingWord(hContact,szMetaProto,"Status",ID_STATUS_OFFLINE);
					iMetaStatusIcon=pcli->pfnIconFromStatusMode(szMetaProto,MetaStatus,NULL);
					if (iIcon==iMetaStatusIcon) //going to set meta icon but need to set most online icon
					{
							hMostMeta=(HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT,(UINT)hContact,0);
							if (hMostMeta!=0)            
							{   
							char * szRealProto;
							int RealStatus;					
						
							szRealProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(UINT)hMostMeta,0);
							RealStatus=DBGetContactSettingWord(hMostMeta,szRealProto,"Status",ID_STATUS_OFFLINE);
							iIcon=pcli->pfnIconFromStatusMode(szRealProto,RealStatus,NULL);

						}
					}
				}
	//clui MS_CLUI_CONTACTADDED MS_CLUI_CONTACTSETICON this methods is null
	saveChangeContactIcon((HANDLE) hContact,(int) iIcon,(int) add);
	//CallService(add?MS_CLUI_CONTACTADDED:MS_CLUI_CONTACTSETICON,(WPARAM)hContact,iIcon);
}

static int GetStatusModeOrdering(int statusMode)
{
	int i;
	for(i=0;i<sizeof(statusModeOrder)/sizeof(statusModeOrder[0]);i++)
		if(statusModeOrder[i].status==statusMode) return statusModeOrder[i].order;
	return 1000;
}

/*void ReLoadContactTree(void)
{
	hideOffline=DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT);

	sortBy[0]=DBGetContactSettingByte(NULL,"CList","SortBy1",SETTING_SORTBY1_DEFAULT);
	sortBy[1]=DBGetContactSettingByte(NULL,"CList","SortBy2",SETTING_SORTBY2_DEFAULT);
	sortBy[2]=DBGetContactSettingByte(NULL,"CList","SortBy3",SETTING_SORTBY3_DEFAULT);
	sortNoOfflineBottom=DBGetContactSettingByte(NULL,"CList","NoOfflineBottom",SETTING_NOOFFLINEBOTTOM_DEFAULT);

	CallService(MS_CLUI_SORTLIST,0,0);
}*/
DWORD CompareContacts2_getLMTime(HANDLE u)
{
	HANDLE hDbEvent;
	DBEVENTINFO dbei = { 0 };

	hDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)u, 0);

	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.pBlob=0;
	dbei.cbBlob=0;
	CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);

	return dbei.timestamp;
}

#define SAFESTRING(a) a?a:""

int GetProtoIndex(char * szName)
{
    DWORD i;
    char buf[11];
    char * name;
    DWORD pc;
    if (!szName) return -1;
    
    pc=DBGetContactSettingDword(NULL,"Protocols","ProtoCount",-1);
    for (i=0; i<pc; i++)
    {
        _itoa(i,buf,10);
        name=DBGetStringA(NULL,"Protocols",buf);
        if (name)
        {
            if (!mir_strcmp(name,szName))
            {
                mir_free(name);
                return i;
            }
            mir_free(name);
        }
    }
    return -1;
}

int CompareContacts2(const struct ClcContact *contact1,const struct ClcContact *contact2, int by)
{

	HANDLE a;
	HANDLE b;
	TCHAR *namea, *nameb;
	int statusa,statusb;
	char *szProto1,*szProto2;
	
	if ((int)contact1<100 || (int)contact2<100) return 0;
	
	a=contact1->hContact;
	b=contact2->hContact;
	
	namea=(TCHAR *)contact1->szText;
	statusa=GetContactCachedStatus(contact1->hContact);
	szProto1=contact1->proto;
	
	nameb=(TCHAR *)contact2->szText;
	statusb=GetContactCachedStatus(contact2->hContact);
	szProto2=contact2->proto;


	if (by==1) { //status
		int ordera,orderb;
		ordera=GetStatusModeOrdering(statusa);
		orderb=GetStatusModeOrdering(statusb);
		if(ordera!=orderb) return ordera-orderb;
		else return 0;
	}


	if(sortNoOfflineBottom==0 && (statusa==ID_STATUS_OFFLINE)!=(statusb==ID_STATUS_OFFLINE)) { //one is offline: offline goes below online
		return 2*(statusa==ID_STATUS_OFFLINE)-1;
	}

	if (by==0) { //name
		return mir_tstrcmpi(namea,nameb);
	} else if (by==2) { //last message
		DWORD ta=CompareContacts2_getLMTime(a);
		DWORD tb=CompareContacts2_getLMTime(b);
		return tb-ta;
	} else if (by==3) {
		int rc=GetProtoIndex(szProto1)-GetProtoIndex(szProto2);

		if (rc != 0 && (szProto1 != NULL && szProto2 != NULL)) return rc;
	}
	// else :o)
	return 0;
}

int cliCompareContacts(const struct ClcContact *contact1,const struct ClcContact *contact2)
{
	int i, r;
	
	for (i=0; i<sizeof(sortBy)/sizeof(char*); i++) {
		r=CompareContacts2(contact1, contact2, sortBy[i]);
		if (r!=0)
			return r;
	}
	return 0;
}

#undef SAFESTRING

static int resortTimerId=0;
static VOID CALLBACK SortContactsTimer(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	KillTimer(NULL,resortTimerId);
	resortTimerId=0;
    if (hwnd!=NULL)
    {    
        KillTimer(hwnd,TIMERID_DELAYEDRESORTCLC);
        SetTimer(hwnd,TIMERID_DELAYEDRESORTCLC,DBGetContactSettingByte(NULL,"CLUI","DELAYEDTIMER",10),NULL);
    }
    else 
	{
	    CallService(MS_CLUI_SORTLIST,0,0);

	}
}

int ContactChangeGroup(WPARAM wParam,LPARAM lParam)
{
	CallService(MS_CLUI_CONTACTDELETED,wParam,0);
	if((HANDLE)lParam==NULL)
		DBDeleteContactSetting((HANDLE)wParam,"CList","Group");
	else
		DBWriteContactSettingTString((HANDLE)wParam,"CList","Group",pcli->pfnGetGroupName(lParam, NULL));
	CallService(MS_CLUI_CONTACTADDED,wParam,ExtIconFromStatusMode((HANDLE)wParam,(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0),GetContactStatus((HANDLE)wParam)));
	return 0;
}

int ToggleHideOffline(WPARAM wParam,LPARAM lParam)
{
	return pcli->pfnSetHideOffline((WPARAM)-1,0);
}
