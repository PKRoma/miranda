#include "..\commonheaders.h"


#define EXTRACOLUMNCOUNT 5
int EnabledColumnCount=0;
boolean visar[EXTRACOLUMNCOUNT];
#define ExtraImageIconsIndexCount 3
int ExtraImageIconsIndex[ExtraImageIconsIndexCount];

static HANDLE hExtraImageListRebuilding,hExtraImageApplying;

extern HWND hwndContactTree;
static HIMAGELIST hExtraImageList;
extern HINSTANCE g_hInst;
extern HIMAGELIST hCListImages;

void SetAllExtraIcons(HWND hwndList,HANDLE hContact);
void LoadExtraImageFunc();
extern HICON LoadIconFromExternalFile (char *filename,int i,boolean UseLibrary,boolean registerit,char *IconName,char *SectName,char *Description,int internalidx);
extern pdisplayNameCacheEntry GetDisplayNameCacheEntry(HANDLE hContact);
boolean ImageCreated=FALSE;
void ReloadExtraIcons();

boolean isColumnVisible(int extra)
{
	switch(extra)
	{
	case EXTRA_ICON_ADV1:		return(DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_ADV1",1));
	case EXTRA_ICON_ADV2:		return(DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_ADV2",1));	
	case EXTRA_ICON_SMS:		return(DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_SMS",1));
	case EXTRA_ICON_EMAIL:		return(DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_EMAIL",1));
	case EXTRA_ICON_PROTO:		return(DBGetContactSettingByte(NULL,CLUIFrameModule,"EXTRA_ICON_PROTO",1));
	};
	return(FALSE);

};

void GetVisColumns()
{
visar[0]=isColumnVisible(EXTRA_ICON_ADV1);
visar[1]=isColumnVisible(EXTRA_ICON_ADV2);
visar[2]=isColumnVisible(EXTRA_ICON_SMS);
visar[3]=isColumnVisible(EXTRA_ICON_EMAIL);
visar[4]=isColumnVisible(EXTRA_ICON_PROTO);

};

__inline int bti(boolean b)
{
	return(b?1:0);
};
int colsum(int from,int to)
{
	int i,sum;
	if (from<0||from>=EXTRACOLUMNCOUNT){return(-1);};
	if (to<0||to>=EXTRACOLUMNCOUNT){return(-1);};
    if (to<from){return(-1);};
	
	sum=0;
	for (i=from;i<=to;i++)
	{
		sum+=bti(visar[i]);
	};
	return(sum);
};
int ExtraToColumnNum(int extra)
{
	int cnt=EnabledColumnCount-1;
	int extracnt=EXTRACOLUMNCOUNT-1;
	
	switch(extra)
	{
	case EXTRA_ICON_ADV1:	if (!visar[0])return(-1);break;
	case EXTRA_ICON_ADV2:	if (!visar[1])return(-1);break;
	case EXTRA_ICON_SMS:	if (!visar[2])return(-1);break;
	case EXTRA_ICON_EMAIL:	if (!visar[3])return(-1);break;
	case EXTRA_ICON_PROTO:	if (!visar[4])return(-1);break;
	};

	switch(extra)
	{
	case EXTRA_ICON_ADV1:if (extracnt-3>=0) return(cnt-colsum(extracnt-3,extracnt));
	case EXTRA_ICON_ADV2:if (extracnt-2>=0) return(cnt-colsum(extracnt-2,extracnt));	
	case EXTRA_ICON_SMS:if (extracnt-1>=0) return(cnt-colsum(extracnt-1,extracnt));
	case EXTRA_ICON_EMAIL:return(cnt-colsum(extracnt,extracnt));
	case EXTRA_ICON_PROTO:return(cnt);
	};
	return(-1);
};

#define ClientNumber 9

char *ClientNames[ClientNumber]=
{"Miranda","RQ","Trillian","GAIM","IM2","Kopete","LICQ","QIP","SIM"};
char *ClientNamesDesc[ClientNumber]=
{"Miranda Client Icon","RQ Client Icon","Trillian Client Icon","GAIM Client Icon","IM2 Client Icon","Kopete Client Icon",
"LICQ Client Icon","QIP Client Icon","SIM Client Icon"};

int ClientICOIDX[ClientNumber]=
{IDI_CLIENTMIRANDA,IDI_CLIENTRQ,IDI_CLIENTTRILLIAN,IDI_CLIENTGAIM,IDI_CLIENTIM2,IDI_CLIENTKOPETE,IDI_CLIENTLICQ,IDI_CLIENTQIP,IDI_CLIENTSIM};


int ClientImageListIdx[ClientNumber]={0};


int GetClientWorker(char *Clientstr,ClientIcon *cli)
{
	if (!cli) return 0;
	cli->ClientID=-1;
	cli->idxClientIcon=-1;
	if (strstr(Clientstr,"Miranda"))
	{
		cli->ClientID=0;cli->idxClientIcon=ClientImageListIdx[0];

		return 0;
	}

	if (strstr(Clientstr,"&RQ"))
	{
		cli->ClientID=1;cli->idxClientIcon=ClientImageListIdx[1];

		return 0;
	}	

	if (strstr(Clientstr,"Trillian"))
	{
		cli->ClientID=2;cli->idxClientIcon=ClientImageListIdx[2];

		return 0;
	}

	if (strstr(Clientstr,"Gaim"))
	{
		cli->ClientID=3;cli->idxClientIcon=ClientImageListIdx[3];

		return 0;
	}

	if (strstr(Clientstr,"IM2"))
	{
		cli->ClientID=4;cli->idxClientIcon=ClientImageListIdx[4];

		return 0;
	}

	if (strstr(Clientstr,"opete"))
	{
		cli->ClientID=5;cli->idxClientIcon=ClientImageListIdx[5];

		return 0;
	}

	if (strstr(Clientstr,"Licq"))
	{
		cli->ClientID=6;cli->idxClientIcon=ClientImageListIdx[6];

		return 0;
	}

	if (strstr(Clientstr,"QIP"))
	{
		cli->ClientID=7;cli->idxClientIcon=ClientImageListIdx[7];

		return 0;
	}
	if (strstr(Clientstr,"SIM"))
	{
		cli->ClientID=8;cli->idxClientIcon=ClientImageListIdx[8];

		return 0;
	}
	return 0;
}


int GetClientIconByMirVer(pdisplayNameCacheEntry pdnce)
{
	if (pdnce&&pdnce->protoNotExists==FALSE&&(!pdnce->isUnknown)&&pdnce->szProto&&pdnce->MirVer)
	{
		if (strlen(pdnce->szProto)>0&&strlen(pdnce->MirVer)>0)
		{
				if (strstr(pdnce->szProto,"ICQ")!=NULL)
				{
					GetClientWorker(pdnce->MirVer,&pdnce->ci);
				}
		}

	}
return 0;
}

void LoadClientIcons()
{

	int i;
	HICON hicon;
	char name[256];
	for (i=0;i<ClientNumber;i++)
	{
		_snprintf(name,sizeof(name),"ClientIcons_%s",ClientNames[i]);
		
		if (ClientImageListIdx[i]==0)
		{
		hicon=LoadIconFromExternalFile("clisticons.dll",i+100,TRUE,TRUE,name,"Contact List",ClientNamesDesc[i],-ClientICOIDX[i]);
		if (hicon) ClientImageListIdx[i]=ImageList_AddIcon(hExtraImageList,hicon );		
		}else
		{
		hicon=LoadIconFromExternalFile("clisticons.dll",i+100,TRUE,FALSE,name,"Contact List",ClientNamesDesc[i],-ClientICOIDX[i]);			
		if (hicon) ClientImageListIdx[i]=ImageList_ReplaceIcon(hExtraImageList,ClientImageListIdx[i],hicon );		
		};
		
	}


}



int SetIconForExtraColumn(WPARAM wParam,LPARAM lParam)
{
	pIconExtraColumn piec;
	int icol;
	HANDLE hItem;

	if (hwndContactTree==0){return(-1);};
	if (wParam==0||lParam==0){return(-1);};
	piec=(pIconExtraColumn)lParam;

	if (piec->cbSize!=sizeof(IconExtraColumn)){return(-1);};
	icol=ExtraToColumnNum(piec->ColumnType);
	if (icol==-1){return(-1);};
	hItem=(HANDLE)SendMessage(hwndContactTree,CLM_FINDCONTACT,(WPARAM)wParam,0);
	if (hItem==0){return(-1);};
	SendMessage(hwndContactTree,CLM_SETEXTRAIMAGE,(WPARAM)hItem,MAKELPARAM(icol,piec->hImage));	
	return(0);
};

//wparam=hIcon
//return hImage on success,-1 on failure
int AddIconToExtraImageList(WPARAM wParam,LPARAM lParam)
{
	if (hExtraImageList==0||wParam==0){return(-1);};
	return((int)ImageList_AddIcon(hExtraImageList,(HICON)wParam));

};

void SetNewExtraColumnCount()
{
	int newcount;

	GetVisColumns();
	newcount=colsum(0,4);
	DBWriteContactSettingByte(NULL,CLUIFrameModule,"EnabledColumnCount",(BYTE)newcount);
	EnabledColumnCount=newcount;
	SendMessage(hwndContactTree,CLM_SETEXTRACOLUMNS,EnabledColumnCount,0);
	
};

int OnIconLibIconChanged(WPARAM wParam,LPARAM lParam)
{
	HICON hicon;
				hicon=LoadIconFromExternalFile("clisticons.dll",0,TRUE,FALSE,"Email","Contact List","Email Icon",-IDI_EMAIL);
				ExtraImageIconsIndex[0]=ImageList_ReplaceIcon(hExtraImageList,ExtraImageIconsIndex[0],hicon );		

				hicon=LoadIconFromExternalFile("clisticons.dll",1,TRUE,FALSE,"Sms","Contact List","Sms Icon",-IDI_SMS);
				ExtraImageIconsIndex[1]=ImageList_ReplaceIcon(hExtraImageList,ExtraImageIconsIndex[1],hicon );		

				hicon=LoadIconFromExternalFile("clisticons.dll",4,TRUE,FALSE,"Web","Contact List","Web Icon",-IDI_GLOBUS);
				ExtraImageIconsIndex[2]=ImageList_ReplaceIcon(hExtraImageList,ExtraImageIconsIndex[2],hicon );		
				
				LoadClientIcons();
				return 0;
};
void ReloadExtraIcons()
{
			{	
				int count,i;
				PROTOCOLDESCRIPTOR **protos;
				HICON hicon;

				SendMessage(hwndContactTree,CLM_SETEXTRACOLUMNSSPACE,DBGetContactSettingByte(NULL,"CLUI","ExtraColumnSpace",18),0);					
				SendMessage(hwndContactTree,CLM_SETEXTRAIMAGELIST,0,(LPARAM)NULL);		
				if (hExtraImageList){ImageList_Destroy(hExtraImageList);};
				//hExtraImageList=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),(IsWinVerXPPlus()?ILC_COLOR32:ILC_COLOR16)|ILC_MASK,1,256);
				
				hExtraImageList=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,1,256);
				memset(ClientImageListIdx,0,sizeof(ClientImageListIdx));

				
				//adding protocol icons
				CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
	
				//loading icons
				hicon=LoadIconFromExternalFile("clisticons.dll",0,TRUE,TRUE,"Email","Contact List","Email Icon",-IDI_EMAIL);
				if (!hicon) hicon=LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_EMAIL));
				ExtraImageIconsIndex[0]=ImageList_AddIcon(hExtraImageList,hicon );

				hicon=LoadIconFromExternalFile("clisticons.dll",1,TRUE,TRUE,"Sms","Contact List","Sms Icon",-IDI_SMS);
				if (!hicon) hicon=LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_SMS));
				ExtraImageIconsIndex[1]=ImageList_AddIcon(hExtraImageList,hicon );

				hicon=LoadIconFromExternalFile("clisticons.dll",4,TRUE,TRUE,"Web","Contact List","Web Icon",-IDI_GLOBUS);
				if (!hicon) hicon=LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_GLOBUS));
				ExtraImageIconsIndex[2]=ImageList_AddIcon(hExtraImageList,hicon );				
				
				
				


				//calc only needed protocols
				for(i=0;i<count;i++) {
				if(protos[i]->type!=PROTOTYPE_PROTOCOL || CallProtoService(protos[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)==0) continue;
				ImageList_AddIcon(hExtraImageList,LoadSkinnedProtoIcon(protos[i]->szName,ID_STATUS_ONLINE));
				}
				LoadClientIcons();
				SendMessage(hwndContactTree,CLM_SETEXTRAIMAGELIST,0,(LPARAM)hExtraImageList);		
				//SetAllExtraIcons(hImgList);
				SetNewExtraColumnCount();
				
				NotifyEventHooks(hExtraImageListRebuilding,0,0);
				ImageCreated=TRUE;
				OutputDebugString("ReloadExtraIcons Done\r\n");
			}

};
void ClearExtraIcons();

void ReAssignExtraIcons()
{
ClearExtraIcons();
SetNewExtraColumnCount();

SetAllExtraIcons(hwndContactTree,0);
SendMessage(hwndContactTree,CLM_AUTOREBUILD,0,0);
};

void ClearExtraIcons()
{
	int i;
	HANDLE hContact,hItem;

	//EnabledColumnCount=DBGetContactSettingByte(NULL,CLUIFrameModule,"EnabledColumnCount",5);
	//SendMessage(hwndContactTree,CLM_SETEXTRACOLUMNS,EnabledColumnCount,0);
	SetNewExtraColumnCount();

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	do {
	
		hItem=(HANDLE)SendMessage(hwndContactTree,CLM_FINDCONTACT,(WPARAM)hContact,0);
		if (hItem==0){continue;};
		for (i=0;i<EnabledColumnCount;i++)
		{
		SendMessage(hwndContactTree,CLM_SETEXTRAIMAGE,(WPARAM)hItem,MAKELPARAM(i,0xFF));	
		};
	
	} while(hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0));
};

void SetAllExtraIcons(HWND hwndList,HANDLE hContact)
{
	HANDLE hItem;
	boolean hcontgiven=FALSE;
	char *szProto;
	char *(ImgIndex[64]);
	int maxpr,count,i;
	PROTOCOLDESCRIPTOR **protos;
	pdisplayNameCacheEntry pdnce;
	int em,pr,sms,a1,a2;
	int tick=0;
	int inphcont=(int)hContact;
	

	hcontgiven=(hContact!=0);

	if (hwndContactTree==0){return;};
	tick=GetTickCount();
	if (ImageCreated==FALSE) ReloadExtraIcons();

	SetNewExtraColumnCount();
	{
		int bla;
		em=ExtraToColumnNum(EXTRA_ICON_EMAIL);	
		pr=ExtraToColumnNum(EXTRA_ICON_PROTO);
		sms=ExtraToColumnNum(EXTRA_ICON_SMS);
		a1=ExtraToColumnNum(EXTRA_ICON_ADV1);
		a2=ExtraToColumnNum(EXTRA_ICON_ADV2);
	bla=123;
			
	
	
	};




//	EnabledColumnCount=DBGetContactSettingByte(NULL,CLUIFrameModule,"EnabledColumnCount",5);
//	SendMessage(hwndContactTree,CLM_SETEXTRACOLUMNS,0,0);
//	SendMessage(hwndContactTree,CLM_SETEXTRACOLUMNS,EnabledColumnCount,0);

				memset(&ImgIndex,0,sizeof(&ImgIndex));
				CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
		
				maxpr=0;
				//calc only needed protocols
				for(i=0;i<count;i++) {
				if(protos[i]->type!=PROTOTYPE_PROTOCOL || CallProtoService(protos[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)==0) continue;
				ImgIndex[maxpr]=protos[i]->szName;
				maxpr++;
				}

	if (hContact==NULL)
	{
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	}	
	
	do {

		szProto=NULL;
		hItem=hContact;
		
		if (hItem==0){continue;};
		pdnce=GetDisplayNameCacheEntry(hItem);
		if (pdnce==NULL) {continue;};
		
//		szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);		
		szProto=pdnce->szProto;

		{
		DBVARIANT dbv={0};
		boolean showweb;	
		showweb=TRUE;
		if (ExtraToColumnNum(EXTRA_ICON_ADV1)!=-1)
		{
			
			if (szProto == NULL || DBGetContactSetting(hContact, szProto, "Homepage",&dbv)!=0) {
						showweb=FALSE;
				}
			
			 PostMessage(hwndList,CLM_SETEXTRAIMAGE,(WPARAM)hItem,MAKELPARAM(ExtraToColumnNum(EXTRA_ICON_ADV1),(showweb)?2:0xFF));	
		if (dbv.pszVal!=NULL) mir_free(dbv.pszVal);
		}
		}		


		{
		DBVARIANT dbv={0};
		boolean showemail;	
		showemail=TRUE;
		if (ExtraToColumnNum(EXTRA_ICON_EMAIL)!=-1)
		{
			
			if (szProto == NULL || DBGetContactSetting(hContact, szProto, "e-mail",&dbv)) {
					if (DBGetContactSetting(hContact, "UserInfo", "Mye-mail0", &dbv))
						showemail=FALSE;
				}
			
			 PostMessage(hwndList,CLM_SETEXTRAIMAGE,(WPARAM)hItem,MAKELPARAM(ExtraToColumnNum(EXTRA_ICON_EMAIL),(showemail)?0:0xFF));	
		if (dbv.pszVal!=NULL) mir_free(dbv.pszVal);
		}
		}

		{
		DBVARIANT dbv={0};
		boolean showsms;	
		showsms=TRUE;
		if (ExtraToColumnNum(EXTRA_ICON_SMS)!=-1)
		{
			if (szProto == NULL || DBGetContactSetting(hContact, szProto, "Cellular",&dbv)) {
					if (DBGetContactSetting(hContact, "UserInfo", "MyPhone0", &dbv))
						showsms=FALSE;
				}
			 PostMessage(hwndList,CLM_SETEXTRAIMAGE,(WPARAM)hItem,MAKELPARAM(ExtraToColumnNum(EXTRA_ICON_SMS),(showsms)?1:0xFF));	
		if (dbv.pszVal!=NULL) mir_free(dbv.pszVal);
		}
		}		

		if(ExtraToColumnNum(EXTRA_ICON_PROTO)!=-1) {

			if(szProto==NULL) {continue;};

			if (pdnce->ci.idxClientIcon!=0&&pdnce->ci.idxClientIcon!=-1)
			{			
							PostMessage(hwndList,CLM_SETEXTRAIMAGE,(WPARAM)hItem,MAKELPARAM(ExtraToColumnNum(EXTRA_ICON_PROTO),pdnce->ci.idxClientIcon));	
			}else
			{					
					for (i=0;i<maxpr;i++)
					{
						if(!strcmp(ImgIndex[i],szProto))
						{

							PostMessage(hwndList,CLM_SETEXTRAIMAGE,(WPARAM)hItem,MAKELPARAM(ExtraToColumnNum(EXTRA_ICON_PROTO),i+3));	
							break;
						};
					};				
			};
		};
		NotifyEventHooks(hExtraImageApplying,(WPARAM)hContact,0);
	  if (hcontgiven) break;

  } while(hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0));
	
	tick=GetTickCount()-tick;
	if (tick>0)
	{
		char buf[256];
		wsprintf(buf,"SetAllExtraIcons %d ms, for %x\r\n",tick,inphcont);
		OutputDebugString(buf);
		DBWriteContactSettingDword((HANDLE)0,"CLUI","PF:Last SetAllExtraIcons Time:",tick);
	}	

  
}



void LoadExtraImageFunc()
{
	CreateServiceFunction(MS_CLIST_EXTRA_SET_ICON,SetIconForExtraColumn); 
	CreateServiceFunction(MS_CLIST_EXTRA_ADD_ICON,AddIconToExtraImageList); 

	hExtraImageListRebuilding=CreateHookableEvent(ME_CLIST_EXTRA_LIST_REBUILD);
	hExtraImageApplying=CreateHookableEvent(ME_CLIST_EXTRA_IMAGE_APPLY);
	HookEvent(ME_SKIN2_ICONSCHANGED,OnIconLibIconChanged);
//	ReloadExtraIcons();
		

};

