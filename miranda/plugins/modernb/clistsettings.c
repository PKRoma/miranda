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
#include "m_clui.h"
#include "m_clc.h"
#include "clist.h"
#include "commonprototypes.h"
#include "hdr/modern_awaymsg.h"

void InsertContactIntoTree(HANDLE hContact,int status);
static displayNameCacheEntry *displayNameCache;

int PostAutoRebuidMessage(HWND hwnd);
static int displayNameCacheSize;

BOOL CLM_AUTOREBUILD_WAS_POSTED=FALSE;
SortedList *clistCache = NULL;
TCHAR* GetNameForContact(HANDLE hContact,int flag,boolean *isUnknown);
char *GetProtoForContact(HANDLE hContact);
int GetStatusForContact(HANDLE hContact,char *szProto);
TCHAR *UnknownConctactTranslatedName=NULL;

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

extern CRITICAL_SECTION LockCacheChain;   //TODO move initialization to cache_func.c module

void InitDisplayNameCache(void)
{
	int i=0;
    InitializeCriticalSection(&LockCacheChain);  //TODO move initialization to cache_func.c module
	InitAwayMsgModule();
	clistCache = li.List_Create( 0, 50 );
	clistCache->sortFunc = handleCompare;
}
void FreeDisplayNameCache()
{
    DeleteCriticalSection(&LockCacheChain);
	UninitAwayMsgModule();
	if ( clistCache != NULL ) {
		int i;
		for ( i = 0; i < clistCache->realCount; i++) {
			pcli->pfnFreeCacheItem(( ClcCacheEntryBase* )clistCache->items[i] );
			mir_free_and_nill( clistCache->items[i] );
		}

		li.List_Destroy( clistCache ); 
		mir_free_and_nill(clistCache);
		clistCache = NULL;
	}	
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

SortedList *CopySmileyString( SortedList *plInput );
void Cache_DestroySmileyList( SortedList *p_list );

void CListSettings_FreeCacheItemData(pdisplayNameCacheEntry pDst)
{
    if (!pDst) return;
    if (pDst->name) 
    {
        if (!pDst->isUnknown && pDst->name!=UnknownConctactTranslatedName) mir_free_and_nill(pDst->name);

    }
    #if defined( _UNICODE )
        if (pDst->szName) mir_free_and_nill(pDst->szName);
	#endif
    if (pDst->szGroup) mir_free_and_nill(pDst->szGroup);
    if (pDst->szSecondLineText) mir_free_and_nill(pDst->szSecondLineText);
    if (pDst->szThirdLineText)  mir_free_and_nill(pDst->szThirdLineText);
    if (pDst->plSecondLineText) {Cache_DestroySmileyList(pDst->plSecondLineText); pDst->plSecondLineText=NULL;}
    if (pDst->plThirdLineText) {Cache_DestroySmileyList(pDst->plThirdLineText); pDst->plThirdLineText=NULL;}
}


int CListSettings_GetCopyFromCache(pdisplayNameCacheEntry pDest);
int CListSettings_SetToCache(pdisplayNameCacheEntry pSrc);

void CListSettings_CopyCacheItems(pdisplayNameCacheEntry pDst, pdisplayNameCacheEntry pSrc)
{
    if (!pDst||!pSrc) return;
    CListSettings_FreeCacheItemData(pDst);
    pDst->name=mir_tstrdup(pSrc->name);
	#if defined( _UNICODE )
        pDst->szName=mir_strdup(pSrc->szName);
	#endif
    pDst->szGroup=mir_tstrdup(pSrc->szGroup);

    pDst->Hidden=pSrc->Hidden;
	pDst->noHiddenOffline=pSrc->noHiddenOffline;
	pDst->szProto=pSrc->szProto;
	pDst->protoNotExists=pSrc->protoNotExists;
	pDst->status=pSrc->status;
	pDst->HiddenSubcontact=pSrc->HiddenSubcontact;
	pDst->i=pSrc->i;
	pDst->ApparentMode=pSrc->ApparentMode;
	pDst->NotOnList=pSrc->NotOnList;
	pDst->IdleTS=pSrc->IdleTS;
	pDst->ClcContact=pSrc->ClcContact;
	pDst->IsExpanded=pSrc->IsExpanded;
	pDst->isUnknown=pSrc->isUnknown;
    pDst->iThirdLineMaxSmileyHeight=pSrc->iThirdLineMaxSmileyHeight;
    pDst->iSecondLineMaxSmileyHeight=pSrc->iSecondLineMaxSmileyHeight;
	pDst->timezone=pSrc->timezone;
    pDst->timediff=pSrc->timediff;
    
    pDst->szSecondLineText=mir_tstrdup(pSrc->szSecondLineText);
    pDst->szThirdLineText=mir_tstrdup(pSrc->szThirdLineText);
      
	if (pSrc->plSecondLineText) pDst->plSecondLineText=CopySmileyString(pSrc->plSecondLineText);  
	if (pSrc->plThirdLineText) pDst->plThirdLineText=CopySmileyString(pSrc->plThirdLineText);
}

int CListSettings_GetCopyFromCache(pdisplayNameCacheEntry pDest)
{
    pdisplayNameCacheEntry pSource;
    if (!pDest || !pDest->hContact) return -1;
    pSource=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(pDest->hContact);
    if (!pSource) return -1;
    CListSettings_CopyCacheItems(pDest, pSource);
    return 0;
}

int CListSettings_SetToCache(pdisplayNameCacheEntry pSrc)
{
    pdisplayNameCacheEntry pDst;
    if (!pSrc || !pSrc->hContact) return -1;
    pDst=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry(pSrc->hContact);
    if (!pDst) return -1;
    CListSettings_CopyCacheItems(pDst, pSrc);
    return 0;
}

void cliFreeCacheItem( pdisplayNameCacheEntry p )
{
	HANDLE hContact=p->hContact;
    TRACEVAR("cliFreeCacheItem hContact=%d",hContact);
	if ( !p->isUnknown && p->name && p->name!=UnknownConctactTranslatedName) mir_free_and_nill(p->name);
	p->name = NULL; 
	#if defined( _UNICODE )
		if ( p->szName) { mir_free_and_nill(p->szName); p->szName = NULL; }
	#endif
	if ( p->szGroup) { mir_free_and_nill(p->szGroup); p->szGroup = NULL; }
	if ( p->szSecondLineText) mir_free_and_nill(p->szSecondLineText);
	if ( p->szThirdLineText) mir_free_and_nill(p->szThirdLineText);
	if ( p->plSecondLineText) {Cache_DestroySmileyList(p->plSecondLineText);p->plSecondLineText=NULL;}
	if ( p->plThirdLineText)  {Cache_DestroySmileyList(p->plThirdLineText);p->plThirdLineText=NULL;}
}


/*
void FreeDisplayNameCache(SortedList *list)
{
	int i;
		for( i=0; i < list->realCount; i++) {
			FreeDisplayNameCacheItem(( pdisplayNameCacheEntry )list->items[i] );
			mir_free_and_nill(list->items[i]);
		}
	li.List_Destroy(list);

}
*/
void cliCheckCacheItem(pdisplayNameCacheEntry pdnce)
{
	if (pdnce!=NULL)
	{
		if (pdnce->hContact==NULL) //selfcontact
		{
			if (!pdnce->name) pdnce->name=GetNameForContact(pdnce->hContact,0,&pdnce->isUnknown);
#ifdef _UNICODE
			if (!pdnce->szName) pdnce->szName=mir_t2a(pdnce->name);
#endif
			return;
		}
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
						if (!pdnce->isUnknown && pdnce->name!=UnknownConctactTranslatedName) mir_free_and_nill(pdnce->name);
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
                if (!pdnce->isUnknown && pdnce->name &&pdnce->name!=UnknownConctactTranslatedName) mir_free_and_nill (pdnce->name);
				if (g_flag_bOnModulesLoadedCalled)
					pdnce->name = GetNameForContact(pdnce->hContact,0,&pdnce->isUnknown); //TODO UNICODE
				else
                    // what to return here???
					pdnce->name = GetNameForContact(pdnce->hContact,0,&pdnce->isUnknown); //TODO UNICODE
			}	
		}
		else
		{
			if (pdnce->isUnknown&&pdnce->szProto&&pdnce->protoNotExists==TRUE&&g_flag_bOnModulesLoadedCalled)
			{
				if (CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)pdnce->szProto)==(int)NULL)
				{
					pdnce->protoNotExists=FALSE;						

					//mir_free_and_nill(pdnce->name);
					pdnce->name= GetNameForContact(pdnce->hContact,0,&pdnce->isUnknown); //TODO UNICODE
				}
			}
		}

		if (pdnce->status==0) //very strange look status sort is broken let always reread status
		{
			pdnce->status=GetStatusForContact(pdnce->hContact,pdnce->szProto);
		}
		if (pdnce->szGroup==NULL)
		{
			DBVARIANT dbv={0};

			if (!DBGetContactSettingTString(pdnce->hContact,"CList","Group",&dbv))
			{
				pdnce->szGroup=mir_tstrdup(dbv.ptszVal);
				//mir_free_and_nill(dbv.ptszVal);
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
		if (pdnce->dwLastMsgTime==0)
		{
			pdnce->dwLastMsgTime=DBGetContactSettingDword(pdnce->hContact, "CList", "mf_lastmsg", 0);
			if (pdnce->dwLastMsgTime==0) pdnce->dwLastMsgTime=CompareContacts2_getLMTime(pdnce->hContact);
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
			if (pdnce->szSecondLineText) 
			{
				
				if (pdnce->plSecondLineText) 
				{
					Cache_DestroySmileyList(pdnce->plSecondLineText);
					pdnce->plSecondLineText=NULL;
				}
				mir_free_and_nill(pdnce->szSecondLineText);
			}
			if (pdnce->szThirdLineText) 
			{
				if (pdnce->plThirdLineText) 
				{
					Cache_DestroySmileyList(pdnce->plThirdLineText);
					pdnce->plThirdLineText=NULL;
				}
				mir_free_and_nill(pdnce->szThirdLineText);
			}
			pdnce->iSecondLineMaxSmileyHeight=0;
			pdnce->iThirdLineMaxSmileyHeight=0;
			pdnce->timediff=0;
			pdnce->timezone=-1;
			pdnce->dwLastMsgTime=0;//CompareContacts2_getLMTime(pdnce->hContact);
			Cache_GetTimezone(NULL,pdnce->hContact);
			SettingType&=~16;
		}

		if (SettingType==-1||SettingType==DBVT_DELETED)
		{	
			if (pdnce->name && !pdnce->isUnknown) mir_free_and_nill(pdnce->name);
			pdnce->name=NULL;
			if (pdnce->szGroup) mir_free_and_nill(pdnce->szGroup);
			// if (pdnce->szProto) mir_free_and_nill(pdnce->szProto);   //free proto
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
			if (pdnce->name && !pdnce->isUnknown) mir_free_and_nill(pdnce->name);
			if (pdnce->szGroup) mir_free_and_nill(pdnce->szGroup);
			//if (pdnce->szProto) mir_free_and_nill(pdnce->szProto);
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
		pdnce->dwLastMsgTime=0;//CompareContacts2_getLMTime(pdnce->hContact);
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
	if (szProto)
		return (int)(DBGetContactSettingWord((HANDLE)hContact,szProto,"Status",ID_STATUS_OFFLINE));
	else 
		return (ID_STATUS_OFFLINE);
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
    if (MirandaExiting() || !pcli || !clistCache || (HANDLE)wParam == NULL) return 0;

	dbv.pszVal = NULL;
	pdnce=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry((HANDLE)wParam);

	if (pdnce==NULL)
	{
		TRACE("!!! Very bad pdnce not found.");
		//if (dbv.pszVal) mir_free_and_nill(dbv.pszVal);
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
					wildcmp((char*)cws->szSetting, (char*) "Status?",2))
			{
				
				if (!mir_strcmp(cws->szModule,"MetaContacts") && mir_strcmp(cws->szSetting, "Status"))
				{
					int res=0;
					//InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);
					if (pcli->hwndContactTree && g_flag_bOnModulesLoadedCalled) 
						res=PostAutoRebuidMessage(pcli->hwndContactTree);
					if ((DBGetContactSettingWord(NULL,"CList","SecondLineType",SETTING_SECONDLINE_TYPE_DEFAULT)==TEXT_STATUS_MESSAGE||DBGetContactSettingWord(NULL,"CList","ThirdLineType",SETTING_THIRDLINE_TYPE_DEFAULT)==TEXT_STATUS_MESSAGE) &&pdnce->hContact && pdnce->szProto)
					{
						//	if (pdnce->status!=ID_STATUS_OFFLINE)  
						amRequestAwayMsg((HANDLE)wParam);  
					}
					DBFreeVariant(&dbv);
					return 0;
				}
				if (!(pdnce->Hidden==1)) 
				{		
					pdnce->status=cws->value.wVal;
					if (cws->value.wVal == ID_STATUS_OFFLINE) 
					{
						if (g_CluiData.bRemoveAwayMessageForOffline)
						{
							char a='\0';
							DBWriteContactSettingString((HANDLE)wParam,"CList","StatusMsg",&a);
						}
					}
					if ((DBGetContactSettingWord(NULL,"CList","SecondLineType",0)==TEXT_STATUS_MESSAGE||DBGetContactSettingWord(NULL,"CList","ThirdLineType",0)==TEXT_STATUS_MESSAGE) &&pdnce->hContact && pdnce->szProto)
					{
						//	if (pdnce->status!=ID_STATUS_OFFLINE)  
						amRequestAwayMsg((HANDLE)wParam);  
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
            if (!strcmp(cws->szSetting,"Rate"))
            {
                pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
            }
			else if (pdnce->name==NULL || !strcmp(cws->szSetting,"MyHandle"))
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
                pcli->pfnClcBroadcast(CLM_AUTOREBUILD,0, 0);
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
		//	mir_free_and_nill(dbv.pszVal);
	} 
	
	return 0;
}

int PostAutoRebuidMessage(HWND hwnd)
{
	if (!CLM_AUTOREBUILD_WAS_POSTED)
		CLM_AUTOREBUILD_WAS_POSTED=PostMessage(hwnd,CLM_AUTOREBUILD,0,0);
	return CLM_AUTOREBUILD_WAS_POSTED;
}
