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

static displayNameCacheEntry *displayNameCache;


static int displayNameCacheSize;

SortedList lContactsCache;
int GetNameForContact(HANDLE hContact,int flag);
char *GetProtoForContact(HANDLE hContact);
int GetStatusForContact(HANDLE hContact,char *szProto);

static int DumpElem( pdisplayNameCacheEntry pdnce )
{
	char buf[256];
	if (pdnce==NULL)
	{
		MessageBox(0,"DumpElem Called with null","",0);
		return (0);
	}
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

void InitDisplayNameCache(SortedList *list)
{
	int i;
	HANDLE hContact;

	memset(list,0,sizeof(SortedList));
	list->dumpFunc =DumpElem;
	list->sortFunc=handleCompare;
	list->sortQsortFunc=handleQsortCompare;
	list->increment=CallService(MS_DB_CONTACT_GETCOUNT,0,0)+1;

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	i=0;
	while (hContact!=0)
	{
			displayNameCacheEntry *pdnce;
			pdnce=mir_calloc(1,sizeof(displayNameCacheEntry));
			pdnce->hContact=hContact;
			List_Insert(list,pdnce,i);
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
		i++;
	}
	
	
		//List_Dump(list);
	List_Sort(list);
	//	List_Dump(list);
}
int gdnc=0;
void FreeDisplayNameCache(SortedList *list)
{
	int i;

	for(i=0;i<(list->realCount);i++)
	{
		pdisplayNameCacheEntry pdnce;
		pdnce=list->items[i];
		if (pdnce&&pdnce->name) mir_free(pdnce->name);
		//if (pdnce&&pdnce->szProto) mir_free(pdnce->szProto);//proto is system string
		if (pdnce&&pdnce->szGroup) mir_free(pdnce->szGroup);
		mir_free(pdnce);
	};
gdnc++;		
	//mir_free(displayNameCache);
	//displayNameCache=NULL;
	//displayNameCacheSize=0;
	
	List_Destroy(list);
	
}

DWORD NameHashFunction(const char *szStr)
{
#if defined _M_IX86 && !defined _NUMEGA_BC_FINALCHECK
	__asm {		   //this breaks if szStr is empty
		xor  edx,edx
		xor  eax,eax
		mov  esi,szStr
		mov  al,[esi]
		xor  cl,cl
	lph_top:	 //only 4 of 9 instructions in here don't use AL, so optimal pipe use is impossible
		xor  edx,eax
		inc  esi
		xor  eax,eax
		and  cl,31
		mov  al,[esi]
		add  cl,5
		test al,al
		rol  eax,cl		 //rol is u-pipe only, but pairable
		                 //rol doesn't touch z-flag
		jnz  lph_top  //5 clock tick loop. not bad.

		xor  eax,edx
	}
#else
	DWORD hash=0;
	int i;
	int shift=0;
	for(i=0;szStr[i];i++) {
		hash^=szStr[i]<<shift;
		if(shift>24) hash^=(szStr[i]>>(32-shift))&0x7F;
		shift=(shift+5)&0x1F;
	}
	return hash;
#endif
}


void CheckPDNCE(pdisplayNameCacheEntry pdnce)
{
	if (pdnce!=NULL)
	{
		if (pdnce->name==NULL)
		{
			pdnce->name=(char *)GetNameForContact(pdnce->hContact,0);
			//pdnce->NameHash=NameHashFunction(pdnce->name);
		}
		if (pdnce->szProto==NULL&&pdnce->protoNotExists==FALSE)
		{
			pdnce->szProto=GetProtoForContact(pdnce->hContact);
			if (pdnce->szProto==NULL) pdnce->protoNotExists=TRUE;

			//pdnce->ProtoHash=NameHashFunction(pdnce->szProto);
		}

		if (pdnce->status==0)
		{
			pdnce->status=GetStatusForContact(pdnce->hContact,pdnce->szProto);
		}
		if (pdnce->szGroup==NULL)
		{
			DBVARIANT dbv;

			if (!DBGetContactSetting(pdnce->hContact,"CList","Group",&dbv))
			{
				pdnce->szGroup=mir_strdup(dbv.pszVal);
				mir_free(dbv.pszVal);
			}else
			{
				pdnce->szGroup=mir_strdup("");
			}

		}
		if (pdnce->Hidden==-1)
		{
			pdnce->Hidden=DBGetContactSettingByte(pdnce->hContact,"CList","Hidden",0);
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


	}
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
	
		if (pdnce!=NULL) CheckPDNCE(pdnce);
		return (pdnce);

	}
}

void InvalidateDisplayNameCacheEntry(HANDLE hContact)
{
	if(hContact==INVALID_HANDLE_VALUE) {
		FreeDisplayNameCache(&lContactsCache);
		InitDisplayNameCache(&lContactsCache);
		SendMessage(hwndContactTree,CLM_AUTOREBUILD,0,0);
	}
	else {
		pdisplayNameCacheEntry pdnce=GetDisplayNameCacheEntry(hContact);
		if (pdnce)
		{
			if (pdnce->name) mir_free(pdnce->name);
			pdnce->name=NULL;
			//if (pdnce->szProto) mir_free(pdnce->szProto); proto is system string
			if (pdnce->szGroup) mir_free(pdnce->szGroup);
			pdnce->szGroup=NULL;
			pdnce->Hidden=-1;
			pdnce->protoNotExists=FALSE;
			pdnce->szProto=NULL;
			pdnce->status=0;
			pdnce->IdleTS=-1;
			pdnce->ApparentMode=-1;
			pdnce->NotOnList=-1;

		};
	}
}
char *GetContactCachedProtocol(HANDLE hContact)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=GetDisplayNameCacheEntry(hContact);
	if (cacheEntry&&cacheEntry->szProto) return cacheEntry->szProto;
	
	return (NULL);
};
char *GetProtoForContact(HANDLE hContact)
{
		char *szProto=NULL;
		szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
		if (szProto==NULL) return(NULL);
		return((szProto));
};

int GetStatusForContact(HANDLE hContact,char *szProto)
{
		int status=ID_STATUS_OFFLINE;
		if (szProto)
		{
				status=DBGetContactSettingWord((HANDLE)hContact,szProto,"Status",ID_STATUS_OFFLINE);
		}
	return (status);
}

int GetNameForContact(HANDLE hContact,int flag)
{
	CONTACTINFO ci;
	char *buffer;

	ZeroMemory(&ci,sizeof(ci));
	ci.cbSize = sizeof(ci);
	ci.hContact = (HANDLE)hContact;
	if (ci.hContact==NULL) ci.szProto = "ICQ";

	if (TRUE)
	{
	
	ci.dwFlag = (int)flag==GCDNF_NOMYHANDLE?CNF_DISPLAYNC:CNF_DISPLAY;
	if (!CallService(MS_CONTACT_GETCONTACTINFO,0,(LPARAM)&ci)) {
		if (ci.type==CNFT_ASCIIZ) {
				buffer = (char*)mir_alloc(strlen(ci.pszVal)+1);
				_snprintf(buffer,strlen(ci.pszVal)+1,"%s",ci.pszVal);
				mir_free(ci.pszVal);
				return (int)buffer;
		}
		if (ci.type==CNFT_DWORD) {

				buffer = (char*)mir_alloc(15);
				_snprintf(buffer,15,"%u",ci.dVal);
				return (int)buffer;
		}
	}
	}
	CallContactService((HANDLE)hContact,PSS_GETINFO,SGIF_MINIMAL,0);
	buffer=Translate("(Unknown Contact)");
	return (int)mir_strdup(buffer);
}

int GetContactHashsForSort(HANDLE hContact,int *ProtoHash,int *NameHash,int *Status)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=GetDisplayNameCacheEntry(hContact);
	if (cacheEntry!=NULL)
	{
		if (ProtoHash!=NULL) *ProtoHash=cacheEntry->ProtoHash;
		if (NameHash!=NULL) *NameHash=cacheEntry->NameHash;
		if (Status!=NULL) *Status=cacheEntry->status;
	}
	return (0);
};

pdisplayNameCacheEntry GetContactFullCacheEntry(HANDLE hContact)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=GetDisplayNameCacheEntry(hContact);
	if (cacheEntry!=NULL)
	{
		return(cacheEntry);
	}
	return (NULL);
}
int GetContactInfosForSort(HANDLE hContact,char **Proto,char **Name,int *Status)
{
	pdisplayNameCacheEntry cacheEntry=NULL;
	cacheEntry=GetDisplayNameCacheEntry(hContact);
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
	cacheEntry=GetDisplayNameCacheEntry(hContact);
	if (cacheEntry&&cacheEntry->status!=0) return cacheEntry->status;
	return (0);	
};

int GetContactDisplayName(WPARAM wParam,LPARAM lParam)
{
	pdisplayNameCacheEntry cacheEntry=NULL;

	if ((int)lParam!=GCDNF_NOMYHANDLE) {
		cacheEntry=GetDisplayNameCacheEntry((HANDLE)wParam);
		if (cacheEntry&&cacheEntry->name) return (int)cacheEntry->name;
		//cacheEntry=NULL;
		//if(displayNameCache[cacheEntry].name) return (int)displayNameCache[cacheEntry].name;
	}
	return (GetNameForContact((HANDLE)wParam,lParam));
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
		InvalidateDisplayNameCacheEntry((HANDLE)wParam);

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