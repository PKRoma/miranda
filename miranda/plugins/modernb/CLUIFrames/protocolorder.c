
// options dialog for protocol order and visibility
// written by daniel vijge
// gpl license ect...
#include "..\commonheaders.h"


extern HINSTANCE g_hInst;
extern char *DBGetStringA(HANDLE hContact,const char *szModule,const char *szSetting);
extern int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam);
extern int MenuModulesLoaded(WPARAM wParam,LPARAM lParam);
extern void ClcOptionsChanged(void);
//extern int ImageList_AddIcon_FixAlpha(HIMAGELIST himl,HICON hicon);


struct ProtocolOrderData {
	int dragging;
	HTREEITEM hDragItem;
};


#define PrVer 3

char **settingname;
int arrlen;

int enumDB_ProtoProc (const char *szSetting,LPARAM lParam)
{
	
	if (szSetting==NULL){return(0);};
	arrlen++;
	settingname=(char **)realloc(settingname,arrlen*sizeof(char *));
	settingname[arrlen-1]=_strdup(szSetting);
	return(0);
};

int  DeleteAllSettingInProtocols()
{
DBCONTACTENUMSETTINGS dbces;
arrlen=0;


dbces.pfnEnumProc=enumDB_ProtoProc;
dbces.szModule="Protocols";
dbces.ofsSettings=0;

CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);

//delete all settings
if (arrlen==0){return(0);};
{
	int i;
	for (i=0;i<arrlen;i++)
	{
	  DBDeleteContactSetting(0,"Protocols",settingname[i]);
	  free(settingname[i]);
	};
	free(settingname);
	settingname=NULL;
};
return(0);
};


int CheckProtocolOrder()
{
	boolean protochanged=FALSE;
	int StoredProtoCount;
	PROTOCOLDESCRIPTOR **protos;
	int i,count;
	int v;
	char *curproto;
	char buf[10];
	int ver;
	
	protochanged=FALSE;
	ver=DBGetContactSettingDword(0,"Protocols","PrVer",-1);
	if (ver!=PrVer){protochanged=TRUE;};
	
	StoredProtoCount=-1;
	if (!protochanged) StoredProtoCount=DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);
	
	if (StoredProtoCount==-1){protochanged=TRUE;};
	if (!protochanged)
	{
		CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);

		v=0;
		//calc only needed protocols
		for(i=0;i<count;i++) {
		if(protos[i]->type!=PROTOTYPE_PROTOCOL || CallProtoService(protos[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)==0) continue;
		v++;
		}
	
		if (StoredProtoCount!=v){protochanged=TRUE;};
	};
	if (!protochanged)
	{
			for(i=0;i<StoredProtoCount;i++) {
				_itoa(i,buf,10);
				curproto=DBGetStringA(NULL,"Protocols",buf);
				if (curproto==NULL){protochanged=TRUE;break;};
				if (CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)curproto)==0)
        {
          protochanged=TRUE;
          if (curproto!=NULL){ mir_free(curproto);}
          break;
        };		
				
				if (curproto!=NULL){ mir_free(curproto);};
			};
	};		
	if (protochanged)
	{
		//reseting all settings;
		DeleteAllSettingInProtocols();

		CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
		
			v=0;
			for(i=0;i<count;i++) {
				if(protos[i]->type!=PROTOTYPE_PROTOCOL || CallProtoService(protos[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)==0) continue;
				_itoa(v,(char *)&buf,10);			
				DBWriteContactSettingString(0,"Protocols",(char *)&buf,protos[i]->szName);
				
				_itoa(OFFSET_PROTOPOS+v,(char *)&buf,10);//save pos in protos
				
				DBDeleteContactSetting(0,"Protocols",(char *)&buf);
				DBWriteContactSettingDword(0,"Protocols",(char *)&buf,v);
				

				_itoa(OFFSET_VISIBLE+v,(char *)&buf,10);//save default visible status
				DBDeleteContactSetting(0,"Protocols",(char *)&buf);
				DBWriteContactSettingDword(0,"Protocols",(char *)&buf,1);
				
			v++;
			};
		DBDeleteContactSetting(0,"Protocols","ProtoCount");
		DBWriteContactSettingDword(0,"Protocols","ProtoCount",v);
		DBWriteContactSettingDword(0,"Protocols","PrVer",PrVer);
			return(1);
	}
	else
	{
		return(0);
	};
	


};

int FillTree(HWND hwnd)
{
			TVINSERTSTRUCT tvis;
			ProtocolData *PD;
			char szName[64];
			char *szSTName;
			char buf[10];
			TCHAR *buf2=NULL;
			int i,count;


			tvis.hParent=NULL;
			tvis.hInsertAfter=TVI_LAST;
			tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	
			
//			CheckProtocolOrder();
			TreeView_DeleteAllItems(hwnd);
			count=DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);
			if (count==-1){return(FALSE);};

			for(i=0;i<count;i++) {
				//if(protos[i]->type!=PROTOTYPE_PROTOCOL || CallProtoService(protos[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)==0) continue;
				_itoa(i,	(char *)&buf,10);
				szSTName=DBGetStringA(0,"Protocols",(char *)&buf);
				if (szSTName==NULL){continue;};
				
				CallProtoService(szSTName,PS_GETNAME,sizeof(szName),(LPARAM)szName);
                
				PD=(ProtocolData*)malloc(sizeof(ProtocolData));
				
				
				PD->RealName=szSTName;
				
				
				_itoa(OFFSET_VISIBLE+i,(char *)&buf,10);
				PD->show=(boolean)DBGetContactSettingDword(0,"Protocols",(char *)&buf,1);

				_itoa(OFFSET_PROTOPOS+i,(char *)&buf,10);
				PD->protopos=DBGetContactSettingDword(0,"Protocols",(char *)&buf,-1);
					
				tvis.item.lParam=(LPARAM)PD;
#ifdef UNICODE
				{
					buf2=a2u(szName);
					tvis.item.pszText=buf2;
				}
#else
				tvis.item.pszText=Translate(szName);
#endif			
				
				tvis.item.iImage=tvis.item.iSelectedImage=PD->show;
				TreeView_InsertItem(hwnd,&tvis);
#ifdef UNICODE
				if (buf2) mir_free(buf2);
#endif
				//tvis.item.iImage=tvis.item.iSelectedImage=PD->show;
				//TreeView_InsertItem(GetDlgItem(hwndDlg,IDC_PROTOCOLVISIBILITY),&tvis);
				//TreeView_InsertItem(GetDlgItem(hwndDlg,IDC_PROTOCOLVISIBILITY),&tvis);
			};
			
			return(0);
};

BOOL CALLBACK ProtocolOrderOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{	struct ProtocolOrderData *dat;

	dat=(struct ProtocolOrderData*)GetWindowLong(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),GWL_USERDATA);

	switch (msg)
	{
		case WM_INITDIALOG: {
			
			TranslateDialogDefault(hwndDlg);
			dat=(struct ProtocolOrderData*)malloc(sizeof(struct ProtocolOrderData));
			SetWindowLong(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),GWL_USERDATA,(long)dat);
			dat->dragging=0;

			//SetWindowLong(GetDlgItem(hwndDlg,IDC_PROTOCOLVISIBILITY),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_PROTOCOLVISIBILITY),GWL_STYLE)|TVS_NOHSCROLL);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),GWL_STYLE)|TVS_NOHSCROLL);

			{	HIMAGELIST himlCheckBoxes;
				himlCheckBoxes=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,2,2);
				ImageList_AddIcon(himlCheckBoxes,LoadSmallIconShared(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_NOTICK)));
				ImageList_AddIcon(himlCheckBoxes,LoadSmallIconShared(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_TICK)));
				TreeView_SetImageList(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),himlCheckBoxes,TVSIL_NORMAL);
			}

			CheckProtocolOrder();
			FillTree(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER));

			return TRUE;
		}
		case WM_COMMAND:
			{
			
			if (HIWORD(wParam)==BN_CLICKED) 
			{
				if (lParam==(LPARAM)GetDlgItem(hwndDlg,IDC_RESETPROTOCOLDATA))	
				{
							DBWriteContactSettingDword(0,"Protocols","ProtoCount",-1);
							CheckProtocolOrder();
							FillTree(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER));
							CluiProtocolStatusChanged(0,0);
							SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
							//BuildStatusMenu(0,0);
				return(0);			
				};
			};
			break;
			};
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {		
			case 0: 
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
						{	
							TVITEMA tvi;
							int count;
							char idstr[33];
							char buf[10];

							tvi.hItem=TreeView_GetRoot(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER));
							tvi.cchTextMax=32;
							tvi.mask=TVIF_TEXT|TVIF_PARAM|TVIF_HANDLE;
							tvi.pszText=(char *)&idstr;
							//CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
							//count--;
							count=0;

							while(tvi.hItem!=NULL) {
								_itoa(count,buf,10);
								TreeView_GetItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&tvi);
								DBWriteContactSettingString(NULL,"Protocols",(char *)&buf,((ProtocolData *)tvi.lParam)->RealName);
								
								_itoa(OFFSET_PROTOPOS+count,(char *)&buf,10);//save pos in protos
								
								DBWriteContactSettingDword(0,"Protocols",(char *)&buf,((ProtocolData *)tvi.lParam)->protopos);

								_itoa(OFFSET_VISIBLE+count,(char *)&buf,10);//save pos in protos
								
								DBWriteContactSettingDword(0,"Protocols",(char *)&buf,((ProtocolData *)tvi.lParam)->show);
								
								tvi.hItem=TreeView_GetNextSibling(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),tvi.hItem);
								
								count++;
							}
							CluiProtocolStatusChanged(0,0);
							MenuModulesLoaded(0,0);
							pcli->pfnTrayIconIconsChanged();
							ClcOptionsChanged();
						}
					}
					break;
					/*
					case IDC_PROTOCOLORDER: //IDC_PROTOCOLVISIBILITY:
					if(((LPNMHDR)lParam)->code==NM_CLICK) {
						TVHITTESTINFO hti;
						hti.pt.x=(short)LOWORD(GetMessagePos());
						hti.pt.y=(short)HIWORD(GetMessagePos());
						ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
						if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti))
							if(hti.flags&TVHT_ONITEMICON) {
								TVITEMA tvi;
								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
								tvi.hItem=hti.hItem;
								TreeView_GetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
								tvi.iImage=tvi.iSelectedImage=!tvi.iImage;
								((ProtocolData *)tvi.lParam)->show=tvi.iImage;
								TreeView_SetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
								SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
								ShowWindowNew(GetDlgItem(hwndDlg,IDC_PROTOCOLORDERWARNING),SW_SHOW);
							}
					}
					break;
					*/
				case IDC_PROTOCOLORDER:
					switch (((LPNMHDR)lParam)->code) {
						case TVN_BEGINDRAGA:
							SetCapture(hwndDlg);
							dat->dragging=1;
							dat->hDragItem=((LPNMTREEVIEW)lParam)->itemNew.hItem;
							TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),dat->hDragItem);
							//ShowWindowNew(GetDlgItem(hwndDlg,IDC_PROTOCOLORDERWARNING),SW_SHOW);
							break;
						case NM_CLICK:
							{
							
						TVHITTESTINFO hti;
						hti.pt.x=(short)LOWORD(GetMessagePos());
						hti.pt.y=(short)HIWORD(GetMessagePos());
						ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
						if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti))
							if(hti.flags&TVHT_ONITEMICON) {
								TVITEMA tvi;
								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
								tvi.hItem=hti.hItem;
								TreeView_GetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
								tvi.iImage=tvi.iSelectedImage=!tvi.iImage;
								((ProtocolData *)tvi.lParam)->show=tvi.iImage;
								TreeView_SetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
								SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
								
								//all changes take effect in runtime
								//ShowWindowNew(GetDlgItem(hwndDlg,IDC_PROTOCOLORDERWARNING),SW_SHOW);
							}
							
							
							
							};
						}
					break;
			}
			break;
		case WM_MOUSEMOVE:
			if(!dat->dragging) break;
			{	TVHITTESTINFO hti;
				hti.pt.x=(short)LOWORD(lParam);
				hti.pt.y=(short)HIWORD(lParam);
				ClientToScreen(hwndDlg,&hti.pt);
				ScreenToClient(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&hti.pt);
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&hti);
				if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT)) {
					hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER))/2;
					TreeView_HitTest(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&hti);
					TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),hti.hItem,1);
				}
				else {
					if(hti.flags&TVHT_ABOVE) SendDlgItemMessage(hwndDlg,IDC_PROTOCOLORDER,WM_VSCROLL,MAKEWPARAM(SB_LINEUP,0),0);
					if(hti.flags&TVHT_BELOW) SendDlgItemMessage(hwndDlg,IDC_PROTOCOLORDER,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),0);
					TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),NULL,0);
				}
			}
			break;
		case WM_LBUTTONUP:
			if(!dat->dragging) break;
			TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),NULL,0);
			dat->dragging=0;
			ReleaseCapture();
			{	TVHITTESTINFO hti;
				TVITEMA tvi;
				hti.pt.x=(short)LOWORD(lParam);
				hti.pt.y=(short)HIWORD(lParam);
				ClientToScreen(hwndDlg,&hti.pt);
				ScreenToClient(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&hti.pt);
				hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER))/2;
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&hti);
				if(dat->hDragItem==hti.hItem) break;
				tvi.mask=TVIF_HANDLE|TVIF_PARAM;
				tvi.hItem=hti.hItem;
				TreeView_GetItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&tvi);
				if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT)) {
					TVINSERTSTRUCTA tvis;
					char name[128];
					tvis.item.mask=TVIF_HANDLE|TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
					tvis.item.stateMask=0xFFFFFFFF;
					tvis.item.pszText=name;
					tvis.item.cchTextMax=sizeof(name);
					tvis.item.hItem=dat->hDragItem;
					//
					tvis.item.iImage=tvis.item.iSelectedImage=((ProtocolData *)tvi.lParam)->show;
					
					TreeView_GetItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&tvis.item);
					

					TreeView_DeleteItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),dat->hDragItem);
					tvis.hParent=NULL;
					tvis.hInsertAfter=hti.hItem;
					TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),TreeView_InsertItem(GetDlgItem(hwndDlg,IDC_PROTOCOLORDER),&tvis));
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
				}
			}
			break; 
	}
	return FALSE;
}

static int ProtocolOrderInit(WPARAM wParam,LPARAM lParam) {
	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-10000000;
    odp.groupPosition=1000000;
	odp.hInstance=g_hInst;//GetModuleHandle(NULL);
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_PROTOCOLORDER);
	odp.pszGroup=Translate("Network");
	odp.pszTitle=Translate("Protocol order");
	odp.pfnDlgProc=ProtocolOrderOpts;
	odp.flags=ODPF_BOLDGROUPS|ODPF_EXPERTONLY;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	return 0;
}

int LoadProtocolOrderModule(void) {
	HookEvent(ME_OPT_INITIALISE,ProtocolOrderInit);
	return 0;
}