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
#include "m_clc.h"
#include "clist.h"
#include "commonprototypes.h"

void InsertContactIntoTree(HANDLE hContact,int status);
static displayNameCacheEntry *displayNameCache;

int PostAutoRebuidMessage(HWND hwnd);
static int displayNameCacheSize;

BOOL CLM_AUTOREBUILD_WAS_POSTED=FALSE;
SortedList *clistCache = NULL;
TCHAR* GetNameForContact(HANDLE hContact,int flag,boolean *isUnknown);
char *GetProtoForContact(HANDLE hContact);
int GetStatusForContact(HANDLE hContact,char *szProto);
TCHAR *UnknownConctactTranslatedName;
extern boolean OnModulesLoadedCalled;
void InvalidateDNCEbyPointer(HANDLE hContact,pdisplayNameCacheEntry pdnce,int SettingType);

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



//	int i, idx;
//	HANDLE hContact;
//
//	memset(list,0,sizeof(SortedList));
//	list->sortFunc = handleCompare;
//	list->increment = 100;
//
//	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
//	i=0;
//	while (hContact!=0)
//	{
//		displayNameCacheEntry *pdnce = mir_calloc(1,sizeof(displayNameCacheEntry));
//		pdnce->hContact = hContact;
//		InvalidateDisplayNameCacheEntryByPDNE(hContact,pdnce,0);
//		li.List_GetIndex(list,pdnce,&idx);
//		li.List_Insert(list,pdnce,idx);
//		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
//		i++;
//}	}
#define MAX_SECTIONS 5

typedef struct _section
{
	HANDLE hContact;
	CRITICAL_SECTION cacheSection;
} LOCKPDNCE;

CRITICAL_SECTION protectSection={0};
LOCKPDNCE lockedPDNCE[MAX_SECTIONS]={0};


void InitDisplayNameCache(void)
{
	int i=0;
	for (i=0;i<MAX_SECTIONS; i++) InitializeCriticalSection(&(lockedPDNCE[i].cacheSection));
	InitializeCriticalSection(&protectSection);
	InitializeCriticalSection(&cacheSection);
	clistCache = li.List_Create( 0, 50 );
	clistCache->sortFunc = handleCompare;
}
void FreeDisplayNameCache()
{
	if ( clistCache != NULL ) {
		int i;
		for ( i = 0; i < clistCache->realCount; i++) {
			pcli->pfnFreeCacheItem(( ClcCacheEntryBase* )clistCache->items[i] );
			mir_free( clistCache->items[i] );
		}

		li.List_Destroy( clistCache ); 
		mir_free(clistCache);
		clistCache = NULL;
		{
			int i=0;
			for (i=0;i<MAX_SECTIONS; i++)	DeleteCriticalSection(&(lockedPDNCE[i].cacheSection));
			DeleteCriticalSection(&protectSection);
			DeleteCriticalSection(&cacheSection);
		}
	}	
}


void LockCacheItem(HANDLE hContact, char * debug, int line)
{
	//
	int ind=0;
	int zero=-1;
	if (!clistCache) return;
	EnterCriticalSection(&protectSection);
	//find here
	for (ind=0; ind<MAX_SECTIONS-1; ind++) 
		if (lockedPDNCE[ind].hContact==hContact) break;
	if (ind==MAX_SECTIONS-1) // was not found 
	{
		for (zero=0; zero<MAX_SECTIONS-1; zero++) if (lockedPDNCE[zero].hContact==NULL) break;
		if (zero!=MAX_SECTIONS-1)
			lockedPDNCE[zero].hContact=hContact;
		ind=zero;
	}
	LeaveCriticalSection(&protectSection);
	//enter here
	//TRACEVAR("In file %s ", debug);
	//TRACEVAR("at line %d ", line);
	//TRACEVAR("locking - %d", hContact);
	EnterCriticalSection(&(lockedPDNCE[ind].cacheSection));
}

void UnlockCacheItem(HANDLE hContact)
{
	int ind=0;
	if (!clistCache) return;
	EnterCriticalSection(&protectSection);
	for (ind=0; ind<MAX_SECTIONS-1; ind++) if (lockedPDNCE[ind].hContact==hContact) break;
	if (ind!=MAX_SECTIONS-1) 
		if (lockedPDNCE[ind].cacheSection.LockCount<1)  lockedPDNCE[ind].hContact=NULL;

	//find here
	LeaveCriticalSection(&protectSection);
	//leave here
	//TRACEVAR(" ... and unlock - %d\n", hContact);
	LeaveCriticalSection(&(lockedPDNCE[ind].cacheSection));
}

ClcCacheEntryBase* cliGetCacheEntry(HANDLE hContact)
{
	ClcCacheEntryBase* p;
	int idx;
	if (!clistCache) return NULL;
	if ( !li.List_GetIndex( clistCache, &hContact, &idx )) {	
		if (( p = pcli->pfnCreateCacheItem( hContact )) != NULL ) {
			li.List_Insert( clistCache, p, idx );
			pcli->pfnInvalidateDisplayNameCacheEntry( hContact );
		}
	}
	else p = ( ClcCacheEntryBase* )clistCache->items[idx];
	pcli->pfnCheckCacheItem( p );
	return p;
}

void cliFreeCacheItem( pdisplayNameCacheEntry p )
{
	HANDLE hContact=p->hContact;
	LockCacheItem(hContact, __FILE__, __LINE__);
	if ( !p->isUnknown && p->name && p->name!=UnknownConctactTranslatedName) mir_free(p->name);
	p->name = NULL; 
	#if defined( _UNICODE )
		if ( p->szName) { mir_free(p->szName); p->szName = NULL; }
	#endif
	if ( p->szGroup) { mir_free(p->szGroup); p->szGroup = NULL; }
	if ( p->szSecondLineText) mir_free(p->szSecondLineText);
	if ( p->szThirdLineText) mir_free(p->szThirdLineText);
	if ( p->plSecondLineText) {Cache_DestroySmileyList(p->plSecondLineText);p->plSecondLineText=NULL;}
	if ( p->plThirdLineText)  {Cache_DestroySmileyList(p->plThirdLineText);p->plThirdLineText=NULL;}
	UnlockCacheItem(hContact);
}


/*
void FreeDisplayNameCache(SortedList *list)
{
	int i;
		for( i=0; i < list->realCount; i++) {
			FreeDisplayNameCacheItem(( pdisplayNameCacheEntry )list->items[i] );
			mir_free(list->items[i]);
		}
	li.List_Destroy(list);

}
*/
void cliCheckCacheItem(pdisplayNameCacheEntry pdnce)
{
	if (pdnce!=NULL)
	{
		if (pdnce->szProto==NULL&&pdnce->protoNotExists==FALSE)
		{
			pdnce->szProto=GetProtoForContact(pdnce->hContact);
			if (pdnce->szProto==NULL) 
			{
				pdnce->protoNotExists=FALSE;
			}else
			{
				if (CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)pdnce->szProto)==(int)NULL &&0)
				{
					pdnce->protoNotExists=TRUE;
				}else
				{
					if(pdnce->szProto&&pdnce->name) 
					{
						if (!pdnce->isUnknown && pdnce->name!=UnknownConctactTranslatedName) mir_free(pdnce->name);
						pdnce->name=NULL;
					}
				}
			}
		}

		if (pdnce->name==NULL)
		{			
			if (pdnce->protoNotExists || !pdnce->szProto)
			{
				pdnce->name=UnknownConctactTranslatedName;
				pdnce->isUnknown=TRUE;
			}
			else
			{
                if (!pdnce->isUnknown && pdnce->name &&pdnce->name!=UnknownConctactTranslatedName) mir_free (pdnce->name);
				if (OnModulesLoadedCalled)
					pdnce->name = GetNameForContact(pdnce->hContact,0,&pdnce->isUnknown); //TODO UNICODE
				else
					pdnce->name = GetNameForContact(pdnce->hContact,0,&pdnce->isUnknown); //TODO UNICODE
			}	
		}
		else
		{
			if (pdnce->isUnknown&&pdnce->szProto&&pdnce->protoNotExists==TRUE&&OnModulesLoadedCalled)
			{
				if (CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)pdnce->szProto)==(int)NULL)
				{
					pdnce->protoNotExists=FALSE;						

					//mir_free(pdnce->name);
					pdnce->name= GetNameForContact(pdnce->hContact,0,&pdnce->isUnknown); //TODO UNICODE
				}
			}
		}

		if (pdnce->status==0)
		{
			pdnce->status=GetStatusForContact(pdnce->hContact,pdnce->szProto);
		}
		if (pdnce->szGroup==NULL)
		{
			DBVARIANT dbv={0};

			if (!DBGetContactSettingTString(pdnce->hContact,"CList","Group",&dbv))
			{
				pdnce->szGroup=mir_tstrdup(dbv.ptszVal);
				//mir_free(dbv.ptszVal);
				DBFreeVariant(&dbv);
			}else
			{
				pdnce->szGroup=mir_tstrdup(TEXT(""));
			}

		}
		if (pdnce->Hidden==-1)
		{
			pdnce->Hidden=DBGetContactSettingByte(pdnce->hContact,"CList","Hidden",0);
		}
		//if (pdnce->HiddenSubcontact==-1)
		//{
		pdnce->HiddenSubcontact=DBGetContactSettingByte(pdnce->hContact,"MetaContacts","IsSubcontact",0);
		//};

		if (pdnce->noHiddenOffline==-1)
		{
			pdnce->noHiddenOffline=DBGetContactSettingByte(pdnce->hContact,"CList","noOffline",0);
		}

		if (pdnce->IdleTS==-1)
		{
			pdnce->IdleTS=DBGetContactSettingDword(pdnce->hContact,pdnce->szProto,"IdleTS",0);
		};

		if (pdnce->ApparentMode==-1)
		{
			pdnce->ApparentMode=DBGetContactSettingWord(pdnce->hContact,pdnce->szProto,"ApparentMode",0);
		};				
		if (pdnce->NotOnList==-1)
		{
			pdnce->NotOnList=DBGetContactSettingByte(pdnce->hContact,"CList","NotOnList",0);
		};		

		if (pdnce->IsExpanded==-1)
		{
			pdnce->IsExpanded=DBGetContactSettingByte(pdnce->hContact,"CList","Expanded",0);
		}
	}
}

void IvalidateDisplayNameCache(DWORD mode)
{
	if ( clistCache != NULL ) 
	{
		int i;
		for ( i = 0; i < clistCache->realCount; i++) 
		{
			PDNCE pdnce=clistCache->items[i];
			if (mode&16)
			{
				InvalidateDNCEbyPointer(pdnce->hContact,pdnce,16);
			}
		}
	}
}

void InvalidateDNCEbyPointer(HANDLE hContact,pdisplayNameCacheEntry pdnce,int SettingType)
{
	if (hContact==NULL) return;
	if (pdnce==NULL) return;
	
	if (pdnce)
	{
		if (SettingType==16)
		{
			LockCacheItem(hContact, __FILE__, __LINE__);
			if (pdnce->szSecondLineText) 
			{
				
				if (pdnce->plSecondLineText) 
				{
					Cache_DestroySmileyList(pdnce->plSecondLineText);
					pdnce->plSecondLineText=NULL;
				}
				mir_free(pdnce->szSecondLineText);
			}
			if (pdnce->szThirdLineText) 
			{
				if (pdnce->plThirdLineText) 
				{
					Cache_DestroySmileyList(pdnce->plThirdLineText);
					pdnce->plThirdLineText=NULL;
				}
				mir_free(pdnce->szThirdLineText);
			}
			pdnce->iSecondLineMaxSmileyHeight=0;
			pdnce->iThirdLineMaxSmileyHeight=0;
			pdnce->timediff=0;
			pdnce->timezone=-1;
			Cache_GetTimezone(NULL,pdnce->hContact);
			SettingType&=~16;
			UnlockCacheItem(hContact);
		}

		if (SettingType==-1||SettingType==DBVT_DELETED)
		{	
			if (pdnce->name && !pdnce->isUnknown) mir_free(pdnce->name);
			pdnce->name=NULL;
			if (pdnce->szGroup) mir_free(pdnce->szGroup);
			// if (pdnce->szProto) mir_free(pdnce->szProto);   //free proto
			pdnce->szGroup=NULL;

			pdnce->Hidden=-1;
			pdnce->HiddenSubcontact=-1;
			pdnce->protoNotExists=FALSE;
			pdnce->szProto=NULL;
			pdnce->status=0;
			pdnce->IdleTS=-1;
			pdnce->ApparentMode=-1;
			pdnce->NotOnList=-1;
			pdnce->isUnknown=FALSE;
			pdnce->noHiddenOffline=-1;
			pdnce->IsExpanded=-1;
			return;
		}
		if (SettingType==DBVT_ASCIIZ||SettingType==DBVT_BLOB)
		{
			if (pdnce->name && !pdnce->isUnknown) mir_free(pdnce->name);
			if (pdnce->szGroup) mir_free(pdnce->szGroup);
			//if (pdnce->szProto) mir_free(pdnce->szProto);
			pdnce->name=NULL;			
			pdnce->szGroup=NULL;
			pdnce->szProto=NULL;
			return;
		}
		// in other cases clear all binary cache
		pdnce->Hidden=-1;
		pdnce->HiddenSubcontact=-1;
		pdnce->protoNotExists=FALSE;
		pdnce->status=0;
		pdnce->IdleTS=-1;
		pdnce->ApparentMode=-1;
		pdnce->NotOnList=-1;
		pdnce->isUnknown=FALSE;
		pdnce->noHiddenOffline=-1;
		pdnce->IsExpanded=-1;
	};
};

char *GetContactCachedProtocol(HANDLE hContact)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hContact);
	if (cacheEntry&&cacheEntry->szProto) return cacheEntry->szProto;

	return (NULL);
}

char* GetProtoForContact(HANDLE hContact)
{
	return (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
}

int GetStatusForContact(HANDLE hContact,char *szProto)
{
	int status=ID_STATUS_OFFLINE;
	if (szProto)
	{
		status=DBGetContactSettingWord((HANDLE)hContact,szProto,"Status",ID_STATUS_OFFLINE);
	}
	return (status);
}

TCHAR* GetNameForContact(HANDLE hContact,int flag,boolean *isUnknown)
{
	TCHAR* result = pcli->pfnGetContactDisplayName(hContact, flag | GCDNF_NOCACHE);
	BOOL itUnknown;
	if (UnknownConctactTranslatedName == NULL)
		UnknownConctactTranslatedName = TranslateT("(Unknown Contact)");
	itUnknown=lstrcmp(result ,UnknownConctactTranslatedName) == 0;
	if (itUnknown) 
		result=UnknownConctactTranslatedName;
	if (isUnknown) {
		*isUnknown = itUnknown;
	}

	return (result);
}

int GetContactInfosForSort(HANDLE hContact,char **Proto,TCHAR **Name,int *Status)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hContact);
	if (cacheEntry!=NULL)
	{
		if (Proto!=NULL) *Proto=cacheEntry->szProto;
		if (Name!=NULL) *Name=cacheEntry->name;
		if (Status!=NULL) *Status=cacheEntry->status;
	}
	return (0);
};


int GetContactCachedStatus(HANDLE hContact)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(hContact);
	if (cacheEntry&&cacheEntry->status!=0) return cacheEntry->status;
	return (0);	
}

int ContactAdded(WPARAM wParam,LPARAM lParam)
{
    if (MirandaExiting()) return 0;
	cli_ChangeContactIcon((HANDLE)wParam,ExtIconFromStatusMode((HANDLE)wParam,(char*)GetContactCachedProtocol((HANDLE)wParam),ID_STATUS_OFFLINE),1); ///by FYR
	pcli->pfnSortContacts();
	return 0;
}

int ContactSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING*)lParam;
	DBVARIANT dbv={0};
	pdisplayNameCacheEntry pdnce;
    if (MirandaExiting()) return 0;
	// Early exit
	if ((HANDLE)wParam == NULL)
		return 0;
	
	dbv.pszVal = NULL;
	pdnce=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry((HANDLE)wParam);

	if (pdnce==NULL)
	{
		TRACE("!!! Very bad pdnce not found.");
		//if (dbv.pszVal) mir_free(dbv.pszVal);
		return 0;
	}
	if (pdnce->protoNotExists==FALSE && pdnce->szProto)
	{
			if (!strcmp(cws->szModule,pdnce->szProto))
		{
			InvalidateDNCEbyPointer((HANDLE)wParam,pdnce,cws->value.type);

				if (!strcmp(cws->szSetting,"IsSubcontact"))
			{
				PostMessage(pcli->hwndContactTree,CLM_AUTOREBUILD,0,0);
			}
			if (!mir_strcmp(cws->szSetting, "Status") ||
					WildCompare((char*)cws->szSetting, (char*) "Status?",2))
			{
				
				if (!mir_strcmp(cws->szModule,"MetaContacts") && mir_strcmp(cws->szSetting, "Status"))
				{
					int res=0;
					//InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);
					if (pcli->hwndContactTree && OnModulesLoadedCalled) 
						res=PostAutoRebuidMessage(pcli->hwndContactTree);
					if ((DBGetContactSettingWord(NULL,"CList","SecondLineType",0)==TEXT_STATUS_MESSAGE||DBGetContactSettingWord(NULL,"CList","ThirdLineType",0)==TEXT_STATUS_MESSAGE) &&pdnce->hContact && pdnce->szProto)
					{
						//	if (pdnce->status!=ID_STATUS_OFFLINE)  
						Cache_ReAskAwayMsg((HANDLE)wParam);  
					}
					DBFreeVariant(&dbv);
					return 0;
				}
				if (!(pdnce->Hidden==1)) 
				{		
					pdnce->status=cws->value.wVal;
					if (cws->value.wVal == ID_STATUS_OFFLINE) 
					{
						//if (DBGetContactSettingByte(NULL,"ModernData","InternalAwayMsgDiscovery",0)
						if (DBGetContactSettingByte(NULL,"ModernData","RemoveAwayMessageForOffline",0))
						{
							char a='\0';
							DBWriteContactSettingString((HANDLE)wParam,"CList","StatusMsg",&a);
						}
					}
					if ((DBGetContactSettingWord(NULL,"CList","SecondLineType",0)==TEXT_STATUS_MESSAGE||DBGetContactSettingWord(NULL,"CList","ThirdLineType",0)==TEXT_STATUS_MESSAGE) &&pdnce->hContact && pdnce->szProto)
					{
						//	if (pdnce->status!=ID_STATUS_OFFLINE)  
						Cache_ReAskAwayMsg((HANDLE)wParam);  
					}
					pcli->pfnClcBroadcast( INTM_STATUSCHANGED,wParam,0);
					cli_ChangeContactIcon((HANDLE)wParam, ExtIconFromStatusMode((HANDLE)wParam,cws->szModule, cws->value.wVal), 0); //by FYR
					pcli->pfnSortContacts();
				}
				else 
				{
					if (!(!mir_strcmp(cws->szSetting, "LogonTS")
						||!mir_strcmp(cws->szSetting, "TickTS")
						||!mir_strcmp(cws->szSetting, "InfoTS")
						))
					{
							pcli->pfnSortContacts();
					}
					DBFreeVariant(&dbv);
					return 0;
				}
			}
		}

		if(!strcmp(cws->szModule,"CList")) 
		{
			//name is null or (setting is myhandle)
			if (pdnce->name==NULL || !strcmp(cws->szSetting,"MyHandle"))
			{
				InvalidateDNCEbyPointer((HANDLE)wParam,pdnce,cws->value.type);
			}
			else if (!strcmp(cws->szSetting,"Group")) 
			{
				InvalidateDNCEbyPointer((HANDLE)wParam,pdnce,cws->value.type);
			}
			else if (!strcmp(cws->szSetting,"Hidden")) 
			{
				InvalidateDNCEbyPointer((HANDLE)wParam,pdnce,cws->value.type);		
				if(cws->value.type==DBVT_DELETED || cws->value.bVal==0) 
				{
					char *szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0);
					//				ChangeContactIcon((HANDLE)wParam,IconFromStatusMode(szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),1);
					cli_ChangeContactIcon((HANDLE)wParam,ExtIconFromStatusMode((HANDLE)wParam,szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),1);  //by FYR
				}
				pcli->pfnClcBroadcast(CLM_AUTOREBUILD,0, 0);
			}
			else if(!strcmp(cws->szSetting,"noOffline")) 
			{
				InvalidateDNCEbyPointer((HANDLE)wParam,pdnce,cws->value.type);		
			}
		}
		else if(!strcmp(cws->szModule,"Protocol")) 
		{
			if(!strcmp(cws->szSetting,"p")) 
			{
				char *szProto;
				InvalidateDNCEbyPointer((HANDLE)wParam,pdnce,cws->value.type);	
				if(cws->value.type==DBVT_DELETED) szProto=NULL;
				else szProto=cws->value.pszVal;
				cli_ChangeContactIcon((HANDLE)wParam,ExtIconFromStatusMode((HANDLE)wParam,szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),0); //by FYR
			}
		}
		// Clean up
		DBFreeVariant(&dbv);
		//if (dbv.pszVal)
		//	mir_free(dbv.pszVal);
	} 
	
	return 0;
}

int PostAutoRebuidMessage(HWND hwnd)
{
	if (!CLM_AUTOREBUILD_WAS_POSTED)
		CLM_AUTOREBUILD_WAS_POSTED=PostMessage(hwnd,CLM_AUTOREBUILD,0,0);
	return CLM_AUTOREBUILD_WAS_POSTED;
}
