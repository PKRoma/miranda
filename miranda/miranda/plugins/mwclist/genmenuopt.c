#include "commonheaders.h"
#pragma hdrstop 



extern int DefaultImageListColorDepth;
extern PIntMenuObject MenuObjects;
extern int MenuObjectsCount;

int hInst;
#define OPTMENU_SEPARATOR "--------------------------------------------------"

struct OrderData {
	int dragging;
	HTREEITEM hDragItem;
};

typedef struct tagMenuItemOptData {
	char *name;
	char *uniqname;
	char *defname;

	int pos;
	boolean show;
	int id;
} 
	MenuItemOptData,*lpMenuItemOptData;

int SaveTree(HWND hwndDlg)
{
	TVITEMA tvi;
	int count;
	char menuItemName[256],DBString[256],MenuNameItems[256];
	char *name;
	int menuitempos,menupos;
	int MenuObjectId,runtimepos;
	PIntMenuObject pimo;

	{
		TVITEMA tvi;
		HTREEITEM hti;

		hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUOBJECTS));
		if (hti==NULL){return(0);}
		tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
		tvi.hItem=hti;
		TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),&tvi);
		MenuObjectId=tvi.lParam;
	}

	tvi.hItem = TreeView_GetRoot(GetDlgItem(hwndDlg,IDC_MENUITEMS));
	tvi.mask = TVIF_PARAM|TVIF_HANDLE;
	count=0;
	//lockbut();

	menupos=GetMenuObjbyId(MenuObjectId);
	if (menupos==-1) return(-1);

	pimo=&MenuObjects[menupos];

	wsprintfA(MenuNameItems, "%s_Items", pimo->Name); 
	runtimepos=0;

	while(tvi.hItem!=NULL) {
		//itoa(count,buf,10);
		TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);
		name=((MenuItemOptData *)tvi.lParam)->name;
#ifdef _DEBUG	
		{
			char buf[256];
			wsprintfA(buf,"savedtree name: %s,id: %d\r\n",name,((MenuItemOptData *)tvi.lParam)->id);
			OutputDebugStringA(buf);
		}
#endif
		menuitempos=GetMenuItembyId(menupos,((MenuItemOptData *)tvi.lParam)->id);
		if (menuitempos!=-1)
		{
			//MenuObjects[MenuObjectId].MenuItems[menuitempos].OverrideShow=
			if (pimo->MenuItems[menuitempos].UniqName)
				wsprintfA(menuItemName,"{%s}",pimo->MenuItems[menuitempos].UniqName);
			else
				wsprintfA(menuItemName,"{%s}",pimo->MenuItems[menuitempos].mi.pszName);

			wsprintfA(DBString, "%s_visible", menuItemName); 
			DBWriteContactSettingByte(NULL, MenuNameItems, DBString, ((MenuItemOptData *)tvi.lParam)->show);

			wsprintfA(DBString, "%s_pos", menuItemName); 								
			DBWriteContactSettingDword(NULL, MenuNameItems, DBString, runtimepos);

			if ( strcmp( ((MenuItemOptData *)tvi.lParam)->name,((MenuItemOptData *)tvi.lParam)->defname )!=0 )
			{
				wsprintfA(DBString, "%s_name", menuItemName); 
				DBWriteContactSettingString(NULL, MenuNameItems, DBString, ((MenuItemOptData *)tvi.lParam)->name);
			}
			runtimepos+=100;
		}

		if ((strcmp(name,OPTMENU_SEPARATOR)==0)&&((MenuItemOptData *)tvi.lParam)->show)
		{
			runtimepos+=SEPARATORPOSITIONINTERVAL;
		}

		tvi.hItem=TreeView_GetNextSibling(GetDlgItem(hwndDlg,IDC_MENUITEMS),tvi.hItem);							
		count++;
	}

	//ulockbut();
	//ttbOptionsChanged();

	return (1);
}

int BuildMenuObjectsTree(HWND hwndDlg)
{
	TVINSERTSTRUCTA tvis;
	int i;

	tvis.hParent=NULL;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	
	TreeView_DeleteAllItems(GetDlgItem(hwndDlg,IDC_MENUOBJECTS));
	if (MenuObjectsCount==0){return(FALSE);}

	for(i=0;i<MenuObjectsCount;i++) {				
		tvis.item.lParam=(LPARAM)MenuObjects[i].id;
		tvis.item.pszText=Translate(MenuObjects[i].Name);
		tvis.item.iImage=tvis.item.iSelectedImage=TRUE;
		SendMessageA(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),TVM_INSERTITEMA,0,(LPARAM)&tvis);			
	}
	return(1);
}

int sortfunc(const void *a,const void *b)
{
	lpMenuItemOptData *sd1,*sd2;
	sd1=(lpMenuItemOptData *)a;
	sd2=(lpMenuItemOptData *)b;
	if ((*sd1)->pos > (*sd2)->pos){return(1);}
	if ((*sd1)->pos < (*sd2)->pos){return(-1);}
	return(0);

}


int InsertSeparator(HWND hwndDlg)
{
	MenuItemOptData *PD;
	TVINSERTSTRUCTA tvis;
	TVITEMA tvi;
	HTREEITEM hti;

	ZeroMemory(&tvi,sizeof(tvi));
	ZeroMemory(&hti,sizeof(hti));
	hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUITEMS));
	if (hti==NULL){return 1;}
	tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_TEXT;
	tvi.hItem=hti;
	if (TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi)==FALSE) return 1;

	ZeroMemory(&tvis,sizeof(tvis));
	PD=(MenuItemOptData*)mir_alloc(sizeof(MenuItemOptData));
	ZeroMemory(PD,sizeof(MenuItemOptData));
	PD->id=-1;
	PD->name=mir_strdup(OPTMENU_SEPARATOR);
	PD->show=TRUE;
	PD->pos=((MenuItemOptData *)tvi.lParam)->pos-1;

	tvis.item.lParam=(LPARAM)(PD);
	tvis.item.pszText=(PD->name);
	tvis.item.iImage=tvis.item.iSelectedImage=PD->show;

	tvis.hParent=NULL;

	tvis.hInsertAfter=hti;
	tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	
	SendMessageA(GetDlgItem(hwndDlg,IDC_MENUITEMS),TVM_INSERTITEMA,0,(LPARAM)&tvis);					
	return(1);
}


int BuildTree(HWND hwndDlg,int MenuObjectId)
{
	struct OrderData *dat;
	TVINSERTSTRUCTA tvis;
	MenuItemOptData *PD;
	lpMenuItemOptData *PDar;
	boolean first;

	char menuItemName[256],MenuNameItems[256];
	char buf[256];
	int i,count,menupos,lastpos;
	PIntMenuObject pimo;


	dat=(struct OrderData*)GetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_USERDATA);
	tvis.hParent=NULL;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	
	TreeView_DeleteAllItems(GetDlgItem(hwndDlg,IDC_MENUITEMS));


	menupos=GetMenuObjbyId(MenuObjectId);
	if (menupos==-1){return(FALSE);}
	pimo=&MenuObjects[menupos];

	if (pimo->MenuItemsCount==0){return(FALSE);}

	wsprintfA(MenuNameItems, "%s_Items", pimo->Name); 

	//*PDar
	count=0;
	for(i=0;i<pimo->MenuItemsCount;i++) {
		if (pimo->MenuItems[i].mi.root!=-1) {continue;}
		count++;
	}

	PDar=mir_alloc(sizeof(lpMenuItemOptData)*count);

	count=0;
	for(i=0;i<pimo->MenuItemsCount;i++) {
		if (pimo->MenuItems[i].mi.root!=-1) {continue;}

		PD=(MenuItemOptData*)mir_alloc(sizeof(MenuItemOptData));
		ZeroMemory(PD,sizeof(MenuItemOptData));
		if (pimo->MenuItems[i].UniqName)
		{

			wsprintfA(menuItemName,"{%s}",pimo->MenuItems[i].UniqName);
		}else
		{
			wsprintfA(menuItemName,"{%s}",pimo->MenuItems[i].mi.pszName);
		}

		{

			DBVARIANT dbv;
			wsprintfA(buf, "%s_name", menuItemName);

			if (!DBGetContactSetting(NULL, MenuNameItems, buf, &dbv))
			{
				PD->name=mir_strdup(dbv.pszVal);			
			}else
			{
				PD->name=mir_strdup(pimo->MenuItems[i].mi.pszName);			
			}
		}

		PD->defname=mir_strdup(pimo->MenuItems[i].mi.pszName);

		wsprintfA(buf, "%s_visible", menuItemName);
		PD->show=DBGetContactSettingByte(NULL,MenuNameItems, buf, 1);			

		wsprintfA(buf, "%s_pos", menuItemName);
		PD->pos=DBGetContactSettingDword(NULL,MenuNameItems, buf, 1);

		PD->id=pimo->MenuItems[i].id;
		{
			char buf[256];
			wsprintfA(buf,"buildtree name: %s,id: %d\r\n",pimo->MenuItems[i].mi.pszName,pimo->MenuItems[i].id);
			OutputDebugStringA(buf);
		}

		if (pimo->MenuItems[i].UniqName) PD->uniqname=mir_strdup(pimo->MenuItems[i].UniqName);

		PDar[count]=PD;
		count++;
	}
	qsort(&(PDar[0]),count,sizeof(lpMenuItemOptData),sortfunc);

	LockWindowUpdate(GetDlgItem(hwndDlg,IDC_MENUITEMS));
	lastpos=0;
	first=TRUE;
	for(i=0;i<count;i++) {

		if (PDar[i]->pos-lastpos>=SEPARATORPOSITIONINTERVAL)
		{
			PD=(MenuItemOptData*)mir_alloc(sizeof(MenuItemOptData));
			ZeroMemory(PD,sizeof(MenuItemOptData));
			PD->id=-1;
			PD->name=mir_strdup(OPTMENU_SEPARATOR);
			PD->pos=PDar[i]->pos-1;
			PD->show=TRUE;

			tvis.item.lParam=(LPARAM)(PD);
			tvis.item.pszText=(PD->name);
			tvis.item.iImage=tvis.item.iSelectedImage=PD->show;
			SendMessageA(GetDlgItem(hwndDlg,IDC_MENUITEMS),TVM_INSERTITEMA,0,(LPARAM)&tvis);					
		}

		tvis.item.lParam=(LPARAM)(PDar[i]);
		tvis.item.pszText=(PDar[i]->name);
		tvis.item.iImage=tvis.item.iSelectedImage=PDar[i]->show;
		{
			HTREEITEM hti = (HTREEITEM)SendMessageA(GetDlgItem(hwndDlg,IDC_MENUITEMS),TVM_INSERTITEMA, 0, (LPARAM)&tvis);
			if (first)
			{
				TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),hti);
				first=FALSE;
			}
		}


		lastpos=PDar[i]->pos;	
	}
	LockWindowUpdate(0);

	mir_free(PDar);

	ShowWindow(GetDlgItem(hwndDlg,IDC_NOTSUPPORTWARNING),(pimo->bUseUserDefinedItems)?SW_HIDE:SW_SHOW);
	EnableWindow(GetDlgItem(hwndDlg,IDC_MENUITEMS),(pimo->bUseUserDefinedItems));
	EnableWindow(GetDlgItem(hwndDlg,IDC_INSERTSEPARATOR),(pimo->bUseUserDefinedItems));

	return 1;
}

void RebuildCurrent(HWND hwndDlg)
{
	TVITEMA tvi;
	HTREEITEM hti;

	hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUOBJECTS));
	if (hti==NULL){return;}
	tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tvi.hItem=hti;
	TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),&tvi);
	BuildTree(hwndDlg,(int)tvi.lParam);
							
}

static BOOL CALLBACK GenMenuOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct OrderData *dat;

	dat=(struct OrderData*)GetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_USERDATA);

	switch (msg) {
		case WM_INITDIALOG: {

			TranslateDialogDefault(hwndDlg);
			dat=(struct OrderData*)mir_alloc(sizeof(struct OrderData));
			SetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_USERDATA,(LONG)dat);
			dat->dragging=0;


			//SetWindowLong(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREE),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREE),GWL_STYLE)|TVS_NOHSCROLL);

			{	HIMAGELIST himlCheckBoxes;
			himlCheckBoxes=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),DefaultImageListColorDepth|ILC_MASK,2,2);
			ImageList_AddIcon(himlCheckBoxes,LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_NOTICK)));
			ImageList_AddIcon(himlCheckBoxes,LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_TICK)));
			//TreeView_SetImageList(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),himlCheckBoxes,TVSIL_NORMAL);
			TreeView_SetImageList(GetDlgItem(hwndDlg,IDC_MENUITEMS),himlCheckBoxes,TVSIL_NORMAL);

			}
			BuildMenuObjectsTree(hwndDlg);
			//			Tree
			//			BuildTree(hwndDlg);
			//			OptionsOpened=TRUE;			

			//			OptionshWnd=hwndDlg;
			return TRUE;
								  }
		case WM_COMMAND:
			{
				if ((HIWORD(wParam)==BN_CLICKED|| HIWORD(wParam)==BN_DBLCLK))
				{
					int ctrlid=LOWORD(wParam);
					if (ctrlid==IDC_INSERTSEPARATOR)
					{
						InsertSeparator(hwndDlg);
						SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					}
					if (ctrlid==IDC_GENMENU_DEFAULT)
					{
						TVITEM tvi;
						HTREEITEM hti;
						char buf[256];

						hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUITEMS));
						if (hti==NULL){break;}
						tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
						tvi.hItem=hti;
						TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);


						if (strstr(((MenuItemOptData *)tvi.lParam)->name,OPTMENU_SEPARATOR)){break;}
						ZeroMemory(buf,256);
						GetDlgItemTextA(hwndDlg,IDC_GENMENU_CUSTOMNAME,buf,256);
						if (((MenuItemOptData *)tvi.lParam)->name){mir_free(((MenuItemOptData *)tvi.lParam)->name);}

						((MenuItemOptData *)tvi.lParam)->name=mir_strdup( ((MenuItemOptData *)tvi.lParam)->defname );

						SaveTree(hwndDlg);
						RebuildCurrent(hwndDlg);

					}

					if (ctrlid==IDC_GENMENU_SET)
					{

						TVITEM tvi;
						HTREEITEM hti;
						char buf[256];

						hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUITEMS));
						if (hti==NULL){break;}
						tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
						tvi.hItem=hti;
						TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);


						if (strstr(((MenuItemOptData *)tvi.lParam)->name,OPTMENU_SEPARATOR)){break;}
						ZeroMemory(buf,256);
						GetDlgItemTextA(hwndDlg,IDC_GENMENU_CUSTOMNAME,buf,256);
						if (((MenuItemOptData *)tvi.lParam)->name){mir_free(((MenuItemOptData *)tvi.lParam)->name);}
						((MenuItemOptData *)tvi.lParam)->name=mir_strdup(buf);

						SaveTree(hwndDlg);
						RebuildCurrent(hwndDlg);


					}
					break;
				}

				if ((HIWORD(wParam)==STN_CLICKED|| HIWORD(wParam)==STN_DBLCLK))
				{
					int ctrlid=LOWORD(wParam);

				}
				break;
			}

		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
			case 0: 
				switch (((LPNMHDR)lParam)->code) {
				case PSN_APPLY:
					{	
						SaveTree(hwndDlg);
						RebuildCurrent(hwndDlg);
					}
				}
				break;

			case IDC_MENUOBJECTS:	
				{
					switch (((LPNMHDR)lParam)->code) {
					case TVN_SELCHANGEDA:
					case TVN_SELCHANGEDW:
						{
							TVITEM tvi;
							HTREEITEM hti;

							hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUOBJECTS));
							if (hti==NULL){break;}
							tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
							tvi.hItem=hti;
							TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),&tvi);
							BuildTree(hwndDlg,(int)tvi.lParam);
							break;
					}	}
					break;
				}
			case IDC_MENUITEMS:
			switch (((LPNMHDR)lParam)->code) {
			case TVN_BEGINDRAGA:
			case TVN_BEGINDRAGW:
				SetCapture(hwndDlg);
				dat->dragging=1;
				dat->hDragItem=((LPNMTREEVIEW)lParam)->itemNew.hItem;
				TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),dat->hDragItem);
				//ShowWindow(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREEWARNING),SW_SHOW);
				break;

			case NM_CLICK:
				{
					TVHITTESTINFO hti;
					hti.pt.x=(short)LOWORD(GetMessagePos());
					hti.pt.y=(short)HIWORD(GetMessagePos());
					ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
					if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti))
					{						
						if(hti.flags&TVHT_ONITEMICON) {
							TVITEM tvi;
							tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
							tvi.hItem=hti.hItem;
							TreeView_GetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
							tvi.iImage=tvi.iSelectedImage=!tvi.iImage;
							((MenuItemOptData *)tvi.lParam)->show=tvi.iImage;
							TreeView_SetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
							SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

							//all changes take effect in runtime
							//ShowWindow(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREEWARNING),SW_SHOW);
						}
					}
				}
				break;
			case TVN_SELCHANGEDA:
			case TVN_SELCHANGEDW:
				{
					TVITEM tvi;
					HTREEITEM hti;

					SetDlgItemTextA(hwndDlg,IDC_GENMENU_CUSTOMNAME,"");
					SetDlgItemTextA(hwndDlg,IDC_GENMENU_SERVICE,"");

					EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_CUSTOMNAME),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_DEFAULT),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_SET),FALSE);

					hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUITEMS));
					if (hti==NULL){
						EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_CUSTOMNAME),FALSE);
						EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_DEFAULT),FALSE);
						EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_SET),FALSE);
						break;}
						tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
						tvi.hItem=hti;
						TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);

						if (strstr(((MenuItemOptData *)tvi.lParam)->name,OPTMENU_SEPARATOR)){break;}

						SetDlgItemTextA(hwndDlg,IDC_GENMENU_CUSTOMNAME,((MenuItemOptData *)tvi.lParam)->name);
						SetDlgItemTextA(hwndDlg,IDC_GENMENU_SERVICE,(((MenuItemOptData *)tvi.lParam)->uniqname)?(((MenuItemOptData *)tvi.lParam)->uniqname):"");

						EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_DEFAULT),TRUE);
						EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_SET),TRUE);
						EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_CUSTOMNAME),TRUE);
				}	}
				break;
			}
			break;
		case WM_MOUSEMOVE:

			if(!dat||!dat->dragging) break;
			{	TVHITTESTINFO hti;
			hti.pt.x=(short)LOWORD(lParam);
			hti.pt.y=(short)HIWORD(lParam);
			ClientToScreen(hwndDlg,&hti.pt);
			ScreenToClient(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti.pt);
			TreeView_HitTest(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti);
			if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT)) {
				hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_MENUITEMS))/2;
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti);
				TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_MENUITEMS),hti.hItem,1);
			}
			else {
				if(hti.flags&TVHT_ABOVE) SendDlgItemMessage(hwndDlg,IDC_MENUITEMS,WM_VSCROLL,MAKEWPARAM(SB_LINEUP,0),0);
				if(hti.flags&TVHT_BELOW) SendDlgItemMessage(hwndDlg,IDC_MENUITEMS,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),0);
				TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_MENUITEMS),NULL,0);
			}

			}
			break;
		case WM_DESTROY:
			{
				//				OptionsOpened=FALSE;
				return(0);
			}

		case WM_LBUTTONUP:
			if(!dat->dragging) break;
			TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_MENUITEMS),NULL,0);
			dat->dragging=0;
			ReleaseCapture();
			{	TVHITTESTINFO hti;
			TVITEM tvi;
			hti.pt.x=(short)LOWORD(lParam);
			hti.pt.y=(short)HIWORD(lParam);
			ClientToScreen(hwndDlg,&hti.pt);
			ScreenToClient(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti.pt);
			hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_MENUITEMS))/2;
			TreeView_HitTest(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti);
			if(dat->hDragItem==hti.hItem) break;
			tvi.mask=TVIF_HANDLE|TVIF_PARAM;
			tvi.hItem=hti.hItem;
			TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);
			if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT)) {
				TVINSERTSTRUCTA tvis;
				char name[128];
				tvis.item.mask=TVIF_HANDLE|TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
				tvis.item.stateMask=0xFFFFFFFF;
				tvis.item.pszText=name;
				tvis.item.cchTextMax=sizeof(name);
				tvis.item.hItem=dat->hDragItem;
				//
				tvis.item.iImage=tvis.item.iSelectedImage=((MenuItemOptData *)tvi.lParam)->show;

				TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvis.item);


				TreeView_DeleteItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),dat->hDragItem);
				tvis.hParent=NULL;
				tvis.hInsertAfter=hti.hItem;
				TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),TreeView_InsertItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvis));
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				{
					SaveTree(hwndDlg);
				}

			}
			}
			break; 
	}
	return FALSE;
}

int GenMenuOptInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;
	//hInst=GetModuleHandle(NULL);

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-1000000000;
	odp.hInstance=g_hInst;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_GENMENU);
	odp.pszGroup=Translate("Customize");
	odp.pszTitle=Translate("MenuOrder");
	odp.pfnDlgProc=GenMenuOpts;
	odp.flags=ODPF_BOLDGROUPS|ODPF_EXPERTONLY;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	return 0;
}