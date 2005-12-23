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
#include "commonprototypes.h"

void InsertContactIntoTree(HANDLE hContact,int status);
static displayNameCacheEntry *displayNameCache;

int PostAutoRebuidMessage(HWND hwnd);
static int displayNameCacheSize;

BOOL CLM_AUTOREBUILD_WAS_POSTED=FALSE;
SortedList lContactsCache;
TCHAR* GetNameForContact(HANDLE hContact,int flag,boolean *isUnknown);
char *GetProtoForContact(HANDLE hContact);
int GetStatusForContact(HANDLE hContact,char *szProto);
TCHAR *UnknownConctactTranslatedName;
extern boolean OnModulesLoadedCalled;
void InvalidateDisplayNameCacheEntryByPDNE(HANDLE hContact,pdisplayNameCacheEntry pdnce,int SettingType);
extern int GetClientIconByMirVer(pdisplayNameCacheEntry pdnce);

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

void FreeDisplayNameCacheItem( pdisplayNameCacheEntry p )
{
	if ( p->name && p->name!=UnknownConctactTranslatedName) mir_free(p->name);
	p->name = NULL; 
	#if defined( _UNICODE )
		if ( p->szName) { mir_free(p->szName); p->szName = NULL; }
	#endif
	if ( p->szGroup) { mir_free(p->szGroup); p->szGroup = NULL; }
	if ( p->MirVer)  { mir_free(p->MirVer);  p->MirVer  = NULL; }
}

void FreeDisplayNameCache(SortedList *list)
{
	int i;

	for( i=0; i < list->realCount; i++) {
		FreeDisplayNameCacheItem(( pdisplayNameCacheEntry )list->items[i] );
		mir_free(list->items[i]);
	}
	
	li.List_Destroy(list);
}

void CheckPDNCE(pdisplayNameCacheEntry pdnce)
{
	if (pdnce!=NULL)
	{
		/*
		{
		char buf[256];
		sprintf(buf,"LoadCacheDispEntry %x \r\n",pdnce);
		TRACE(buf);
		}
		*/
		if (pdnce->szProto==NULL&&pdnce->protoNotExists==FALSE)
		{
			//if (pdnce->szProto) mir_free(pdnce->szProto);
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
						if (pdnce->name!=UnknownConctactTranslatedName) mir_free(pdnce->name);
						pdnce->name=NULL;
					}
				}
			}
		}

		if (pdnce->name==NULL)
		{			
			if (pdnce->protoNotExists)
			{
				pdnce->name=mir_strdupT(TranslateT("_NoProtocol_"));
			}
			else
			{
				if (OnModulesLoadedCalled)
					pdnce->name = GetNameForContact(pdnce->hContact,0,&pdnce->isUnknown); //TODO UNICODE
				else
					pdnce->name = GetNameForContact(pdnce->hContact,0,NULL); //TODO UNICODE
			}	
		}
		else
		{
			if (pdnce->isUnknown&&pdnce->szProto&&pdnce->protoNotExists==TRUE&&OnModulesLoadedCalled)
			{
				if (CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)pdnce->szProto)==(int)NULL)
				{
					pdnce->protoNotExists=FALSE;						

					mir_free(pdnce->name);
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
			DBVARIANT dbv;

			if (!DBGetContactSettingTString(pdnce->hContact,"CList","Group",&dbv))
			{
				pdnce->szGroup=mir_strdupT(dbv.ptszVal);
				mir_free(dbv.ptszVal);
				DBFreeVariant(&dbv);
			}else
			{
				pdnce->szGroup=mir_strdupT(TEXT(""));
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

		if (pdnce->MirVer==NULL||pdnce->ci.idxClientIcon==-2||pdnce->ci.idxClientIcon==-2)
		{
			if (pdnce->MirVer) mir_free(pdnce->MirVer);
			pdnce->MirVer=DBGetStringA(pdnce->hContact,pdnce->szProto,"MirVer");
			if (!pdnce->MirVer) pdnce->MirVer=mir_strdup("");
			pdnce->ci.ClientID=-1;
			pdnce->ci.idxClientIcon=-1;
			if (pdnce->MirVer!=NULL)
			{
				GetClientIconByMirVer(pdnce);
			}

		}
	}
}

void InvalidateDisplayNameCacheEntryByPDNE(HANDLE hContact,pdisplayNameCacheEntry pdnce,int SettingType)
{
	if (hContact==NULL) return;
	if (pdnce==NULL) return;
	if (pdnce)
	{
		if (SettingType==-1||SettingType==DBVT_DELETED)
		{		
			if (pdnce->name) mir_free(pdnce->name);
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
			if (pdnce->MirVer) mir_free(pdnce->MirVer);
			pdnce->MirVer=NULL;

			pdnce->ci.ClientID=-2;
			pdnce->ci.idxClientIcon=-2;

			return;
		}
		if (SettingType==DBVT_ASCIIZ||SettingType==DBVT_BLOB)
		{
			if (pdnce->name) mir_free(pdnce->name);
			if (pdnce->szGroup) mir_free(pdnce->szGroup);
			//if (pdnce->szProto) mir_free(pdnce->szProto);
			if (pdnce->MirVer) mir_free(pdnce->MirVer);			
			pdnce->name=NULL;			
			pdnce->szGroup=NULL;
			pdnce->szProto=NULL;				
			pdnce->MirVer=NULL;
			pdnce->ci.ClientID=-2;
			pdnce->ci.idxClientIcon=-2;

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
		//Can cause start delay
   		    if (pdnce->MirVer) mir_free(pdnce->MirVer);
			pdnce->MirVer=NULL;
			pdnce->ci.ClientID=-2;
			pdnce->ci.idxClientIcon=-2;

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
	if (itUnknown) result=UnknownConctactTranslatedName;
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
	ChangeContactIcon((HANDLE)wParam,ExtIconFromStatusMode((HANDLE)wParam,(char*)GetContactCachedProtocol((HANDLE)wParam),ID_STATUS_OFFLINE),1); ///by FYR
	//	ChangeContactIcon((HANDLE)wParam,pcli->pfnIconFromStatusMode((char*)GetContactCachedProtocol((HANDLE)wParam),ID_STATUS_OFFLINE),1);
	pcli->pfnSortContacts();
	return 0;
}

extern void ReAskStatusMessage(HANDLE wParam);

int ContactSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING*)lParam;
	DBVARIANT dbv;
	pdisplayNameCacheEntry pdnce;

	// Early exit
	if ((HANDLE)wParam == NULL)
		return 0;
	
	dbv.pszVal = NULL;
	pdnce=(pdisplayNameCacheEntry)pcli->pfnGetCacheEntry((HANDLE)wParam);

	if (pdnce==NULL)
	{
		TRACE("!!! Very bad pdnce not found.");
		if (dbv.pszVal) mir_free(dbv.pszVal);
		return 0;
	}
	if (pdnce->protoNotExists==FALSE && pdnce->szProto)
	{
			if (!strcmp(cws->szModule,pdnce->szProto))
		{
			InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);

				if (!strcmp(cws->szSetting,"IsSubcontact"))
			{
				PostMessage(pcli->hwndContactTree,CLM_AUTOREBUILD,0,0);
			}
			if (!MyStrCmp(cws->szSetting, "Status") ||
					WildCompare((char*)cws->szSetting, (char*) "Status?",2))
			{
				
				if (!MyStrCmp(cws->szModule,"MetaContacts") && MyStrCmp(cws->szSetting, "Status"))
				{
					int res=0;
					//InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);
					if (pcli->hwndContactTree && OnModulesLoadedCalled) 
						res=PostAutoRebuidMessage(pcli->hwndContactTree);
					if ((DBGetContactSettingWord(NULL,"CList","SecondLineType",0)==TEXT_STATUS_MESSAGE||DBGetContactSettingWord(NULL,"CList","ThirdLineType",0)==TEXT_STATUS_MESSAGE) &&pdnce->hContact && pdnce->szProto)
					{
						//	if (pdnce->status!=ID_STATUS_OFFLINE)  
						ReAskStatusMessage((HANDLE)wParam);  
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
						ReAskStatusMessage((HANDLE)wParam);  
					}
					TRACE("From ContactSettingChanged: ");
					pcli->pfnClcBroadcast( INTM_STATUSCHANGED,wParam,0);
					ChangeContactIcon((HANDLE)wParam, ExtIconFromStatusMode((HANDLE)wParam,cws->szModule, cws->value.wVal), 0); //by FYR
					pcli->pfnSortContacts();
				}
				else 
				{
					if (!(!MyStrCmp(cws->szSetting, "LogonTS")
						||!MyStrCmp(cws->szSetting, "TickTS")
						||!MyStrCmp(cws->szSetting, "InfoTS")
						))
					{
						pcli->pfnSortContacts();
					}
					DBFreeVariant(&dbv);
					return 0;
				}
				//TRACE("Second SortContact\n");
			}
		}

		if(!strcmp(cws->szModule,"CList")) 
		{
			//name is null or (setting is myhandle)
			if (pdnce->name==NULL || !strcmp(cws->szSetting,"MyHandle"))
			{
				InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);
			}
			else if (!strcmp(cws->szSetting,"Group")) 
			{
				InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);
			}
			else if (!strcmp(cws->szSetting,"Hidden")) 
			{
				InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);		
				if(cws->value.type==DBVT_DELETED || cws->value.bVal==0) 
				{
					char *szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0);
					//				ChangeContactIcon((HANDLE)wParam,IconFromStatusMode(szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),1);
					ChangeContactIcon((HANDLE)wParam,ExtIconFromStatusMode((HANDLE)wParam,szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),1);  //by FYR
				}
			}
			else if(!strcmp(cws->szSetting,"noOffline")) 
			{
				InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);		
			}
		}
		else if(!strcmp(cws->szModule,"Protocol")) 
		{
			if(!strcmp(cws->szSetting,"p")) 
			{
				char *szProto;

				TRACE("CHANGE: proto\r\n");
				InvalidateDisplayNameCacheEntryByPDNE((HANDLE)wParam,pdnce,cws->value.type);	
				if(cws->value.type==DBVT_DELETED) szProto=NULL;
				else szProto=cws->value.pszVal;
				ChangeContactIcon((HANDLE)wParam,ExtIconFromStatusMode((HANDLE)wParam,szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),0); //by FYR
				//			ChangeContactIcon((HANDLE)wParam,IconFromStatusMode(szProto,szProto==NULL?ID_STATUS_OFFLINE:DBGetContactSettingWord((HANDLE)wParam,szProto,"Status",ID_STATUS_OFFLINE)),0);
			}
		}

		// Clean up
		if (dbv.pszVal)
			mir_free(dbv.pszVal);
	} 
	
	return 0;
}

int PostAutoRebuidMessage(HWND hwnd)
{
	if (!CLM_AUTOREBUILD_WAS_POSTED)
		CLM_AUTOREBUILD_WAS_POSTED=PostMessage(hwnd,CLM_AUTOREBUILD,0,0);
	return CLM_AUTOREBUILD_WAS_POSTED;
}
