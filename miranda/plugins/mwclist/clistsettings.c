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
#include "m_clc.h"
#include "clist.h"
#include "dblists.h"

void InsertContactIntoTree(HANDLE hContact,int status);
extern HWND hwndContactTree;

typedef struct  {
	HANDLE hContact;
	char *name;
	char *szProto;
	int	  status;
	int i;
} displayNameCacheEntry,*pdisplayNameCacheEntry;


static displayNameCacheEntry *displayNameCache;


static int displayNameCacheSize;

SortedList lContactsCache;

static int DumpElem( pdisplayNameCacheEntry pdnce )
{
	char buf[256];
	if (pdnce->name) sprintf(buf,"%x: hc:%x, %s\r\n",pdnce,pdnce->hContact,pdnce->name);
		else
		sprintf(buf,"%x: hc:%x\r\n",pdnce,pdnce->hContact);

	OutputDebugString(buf);
	return 0;
};



static int handleCompare( void* c1, void* c2 )
{
		
	int p1, p2;

	displayNameCacheEntry	*dnce1=c1;
	displayNameCacheEntry	*dnce2=c2;
	
	p1=(int)dnce1->hContact;
	p2=(int)dnce2->hContact;

	if ( p1 == p2 )
		return 0;

	return p1 - p2;
}
static int handleQsortCompare( void** c1, void** c2 )
{
	//heh damn c pointer writing...i only want to call handleCompare(c1^,c2^) as in pascal.
	return (handleCompare((void *)*( long *)c1,(void *)*( long *)c2));		
}

void InitDisplayNameCache(void)
{
	int i;
	HANDLE hContact;
	lContactsCache.dumpFunc =DumpElem;
	lContactsCache.sortFunc=handleCompare;
	lContactsCache.sortQsortFunc=handleQsortCompare;
	lContactsCache.increment=1024;

	displayNameCacheSize=CallService(MS_DB_CONTACT_GETCOUNT,0,0)+1;
	
	displayNameCache=(displayNameCacheEntry*)mir_realloc(NULL,displayNameCacheSize*sizeof(displayNameCacheEntry));

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	i=0;
	while (hContact!=0)
	{
			displayNameCacheEntry *pdnce;
			pdnce=mir_calloc(1,sizeof(displayNameCacheEntry));
			pdnce->hContact=hContact;
			//pdnce->hContact=i*1+1;
			//dnce->name=_strdup(displayNameCache[i].name);

			List_Insert(&lContactsCache,pdnce,i);
	
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
		i++;
	}
	
	
	//List_Dump(&lContactsCache);
	List_Sort(&lContactsCache);
	//List_Dump(&lContactsCache);
}
int gdnc=0;
void FreeDisplayNameCache(void)
{
	int i;

	for(i=0;i<(lContactsCache.realCount);i++)
	{
		pdisplayNameCacheEntry pdnce;
		pdnce=lContactsCache.items[i];
		if (pdnce&&pdnce->name) mir_free(pdnce->name);
		if (pdnce&&pdnce->szProto) mir_free(pdnce->szProto);
		mir_free(pdnce);
	};
gdnc++;		
	//mir_free(displayNameCache);
	//displayNameCache=NULL;
	//displayNameCacheSize=0;
	
	List_Destroy(&lContactsCache);
	
}

static pdisplayNameCacheEntry GetDisplayNameCacheEntry(HANDLE hContact)
{
	{
	displayNameCacheEntry dnce, *pdnce,*pdnce2;
	
	dnce.hContact=hContact;
	pdnce=List_Find(&lContactsCache,&dnce);

		if (pdnce==NULL)
		{
			pdnce=mir_calloc(1,sizeof(displayNameCacheEntry));
			pdnce->hContact=hContact;
			List_Insert(&lContactsCache,pdnce,0);
//List_Dump(&lContactsCache);
			List_Sort(&lContactsCache);
//List_Dump(&lContactsCache);
			pdnce2=List_Find(&lContactsCache,&dnce);//for check
			if (pdnce2->hContact!=pdnce->hContact)
			{
				return (NULL);
			};

			if (pdnce2!=pdnce)
			{
				return (NULL);
			}
		};
	
		return (pdnce);

	}
}

void InvalidateDisplayNameCacheEntry(HANDLE hContact)
{
	if(hContact==INVALID_HANDLE_VALUE) {
		FreeDisplayNameCache();
		InitDisplayNameCache();
		SendMessage(hwndContactTree,CLM_AUTOREBUILD,0,0);
	}
	else {
		pdisplayNameCacheEntry pdnce=GetDisplayNameCacheEntry(hContact);
		if (pdnce)
		{
			if (pdnce->name) mir_free(pdnce->name);
			pdnce->name=NULL;
			if (pdnce->szProto) mir_free(pdnce->szProto);
			pdnce->szProto=NULL;
			pdnce->status=0;
		};
	}
}
char *GetContactCachedProtocol(HANDLE hContact)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=GetDisplayNameCacheEntry(hContact);
	if (cacheEntry&&cacheEntry->szProto) return cacheEntry->szProto;
	
	{
		char *szProto=NULL;
		szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
		if (szProto)
		{
		if (cacheEntry)
			{
				cacheEntry->szProto=mir_strdup(szProto);
			}
		}
	return(szProto);
	}
	
};

int GetContactCachedStatus(HANDLE hContact)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=GetDisplayNameCacheEntry(hContact);
	if (cacheEntry&&cacheEntry->status!=0) return cacheEntry->status;
	{
		int status=ID_STATUS_OFFLINE;
		char *szProto;
		szProto=GetContactCachedProtocol(hContact);
		if (szProto)
		{
				status=DBGetContactSettingWord((HANDLE)hContact,szProto,"Status",ID_STATUS_OFFLINE);
				if (cacheEntry)
					{
						cacheEntry->status=status;
					}
		}
		return(status);
	}
	
};

int GetContactDisplayName(WPARAM wParam,LPARAM lParam)
{
	CONTACTINFO ci;
	pdisplayNameCacheEntry cacheEntry=NULL;
	char *buffer;
	
	

	if ((int)lParam!=GCDNF_NOMYHANDLE) {
		cacheEntry=GetDisplayNameCacheEntry((HANDLE)wParam);
		if (cacheEntry&&cacheEntry->name) return (int)cacheEntry->name;
		//cacheEntry=NULL;
		//if(displayNameCache[cacheEntry].name) return (int)displayNameCache[cacheEntry].name;
	}
	ZeroMemory(&ci,sizeof(ci));
	ci.cbSize = sizeof(ci);
	ci.hContact = (HANDLE)wParam;
	if (ci.hContact==NULL) ci.szProto = "ICQ";
	/*
	if (ci.hContact==NULL)
	{
		return (int)mir_strdup(Translate("Owner"));
	}
	*/
	if (TRUE/*ci.hContact!=NULL*/)
	{
	
	ci.dwFlag = (int)lParam==GCDNF_NOMYHANDLE?CNF_DISPLAYNC:CNF_DISPLAY;
	if (!CallService(MS_CONTACT_GETCONTACTINFO,0,(LPARAM)&ci)) {
		if (ci.type==CNFT_ASCIIZ) {
			if (cacheEntry==NULL) {
				buffer = (char*)mir_alloc(strlen(ci.pszVal)+1);
				_snprintf(buffer,strlen(ci.pszVal)+1,"%s",ci.pszVal);
				mir_free(ci.pszVal);
				//DBFreeVariant(&ci.pszVal)
				return (int)buffer;
			}
			else {
				cacheEntry->name = mir_strdup(ci.pszVal);
				return (int)cacheEntry->name;
			}
		}
		if (ci.type==CNFT_DWORD) {
			if (cacheEntry==NULL) {
				buffer = (char*)mir_alloc(15);
				_snprintf(buffer,15,"%u",ci.dVal);
				return (int)buffer;
			}
			else {
				buffer = (char*)mir_alloc(15);
				_snprintf(buffer,15,"%u",ci.dVal);
				cacheEntry->name = mir_strdup(buffer);
				return (int)buffer;
			}
		}
	}
	}
	CallContactService((HANDLE)wParam,PSS_GETINFO,SGIF_MINIMAL,0);
	buffer=Translate("(Unknown Contact)");
	if (cacheEntry!=NULL) cacheEntry->name=mir_strdup(buffer);
	return (int)buffer;
}

int InvalidateDisplayName(WPARAM wParam,LPARAM lParam) {
	InvalidateDisplayNameCacheEntry((HANDLE)wParam);
	return 0;
}

int ContactAdded(WPARAM wParam,LPARAM lParam)
{
	ChangeContactIcon((HANDLE)wParam,IconFromStatusMode((char*)GetContactCachedProtocol((HANDLE)wParam),ID_STATUS_OFFLINE),1);
	SortContacts();
	return 0;
}

int ContactDeleted(WPARAM wParam,LPARAM lParam)
{
	CallService(MS_CLUI_CONTACTDELETED,wParam,0);
	return 0;
}

int ContactSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING*)lParam;
	DBVARIANT dbv;

	// Early exit
	if ((HANDLE)wParam == NULL)
		return 0;

	dbv.pszVal = NULL;
	if (!DBGetContactSetting((HANDLE)wParam, "Protocol", "p", &dbv))
	{
		if (!strcmp(cws->szModule,dbv.pszVal))
		{
			InvalidateDisplayNameCacheEntry((HANDLE)wParam);
			if (!strcmp(cws->szSetting,"UIN") || !strcmp(cws->szSetting,"Nick") || !strcmp(cws->szSetting,"FirstName") || !strcmp(cws->szSetting,"LastName") || !strcmp(cws->szSetting,"e-mail"))
			{
				CallService(MS_CLUI_CONTACTRENAMED,wParam, 0);
			}
			else if (!strcmp(cws->szSetting, "Status")) {
				if (!DBGetContactSettingByte((HANDLE)wParam, "CList", "Hidden", 0)) {
					if (DBGetContactSettingByte(NULL, "CList", "HideOffline", SETTING_HIDEOFFLINE_DEFAULT))	{
						// User's state is changing, and we are hideOffline-ing
						if (cws->value.wVal == ID_STATUS_OFFLINE) {
							ChangeContactIcon((HANDLE)wParam, IconFromStatusMode(cws->szModule, cws->value.wVal), 0);
							CallService(MS_CLUI_CONTACTDELETED, wParam, 0);
							mir_free(dbv.pszVal);
							return 0;
						}
						ChangeContactIcon((HANDLE)wParam, IconFromStatusMode(cws->szModule, cws->value.wVal), 1);
					}
					ChangeContactIcon((HANDLE)wParam, IconFromStatusMode(cws->szModule, cws->value.wVal), 0);
				}
			}
			else
			{
			
				DBFreeVariant(&dbv);
				return 0;
			}
			SortContacts();
		}
	}

	if(!strcmp(cws->szModule,"CList")) {
		if(!strcmp(cws->szSetting,"Hidden")) {
			if(cws->value.type==DBVT_DELETED || cws->value.bVal==0) {
				char *szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0);
				ChangeContactIcon((HANDLE)wParam,IconFromStatusMode(szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),1);
			}
			else
				CallService(MS_CLUI_CONTACTDELETED,wParam,0);
		}
		if(!strcmp(cws->szSetting,"MyHandle")) {
			InvalidateDisplayNameCacheEntry((HANDLE)wParam);
		}
	}

	if(!strcmp(cws->szModule,"Protocol")) {
		if(!strcmp(cws->szSetting,"p")) {
			char *szProto;
			if(cws->value.type==DBVT_DELETED) szProto=NULL;
			else szProto=cws->value.pszVal;
			ChangeContactIcon((HANDLE)wParam,IconFromStatusMode(szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),0);
		}
	}

	// Clean up
	if (dbv.pszVal)
		mir_free(dbv.pszVal);

 	return 0;

}