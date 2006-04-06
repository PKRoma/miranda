#include "commonheaders.h"
#include "m_genmenu.h"
#include "genmenu.h"
#include "clist.h"
#pragma hdrstop 

#define TreeView_InsertItemA(hwnd, lpis) \
	(HTREEITEM)SendMessageA((hwnd), TVM_INSERTITEMA, 0, (LPARAM)(LPTV_INSERTSTRUCTA)(lpis))

#define TreeView_GetItemA(hwnd, pitem) \
	(BOOL)SendMessageA((hwnd), TVM_GETITEMA, 0, (LPARAM)(TV_ITEM *)(pitem))


extern PIntMenuObject MenuObjects;
extern int MenuObjectsCount;
//extern int ImageList_AddIcon_FixAlpha(HIMAGELIST himl,HICON hicon);
HTREEITEM MoveItemAbove(HWND hTreeWnd, HTREEITEM hItem, HTREEITEM hInsertAfter);
long handleCustomDraw(HWND hWndTreeView, LPNMTVCUSTOMDRAW pNMTVCD);

int hInst;

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
        DWORD isSelected;
		int id;
} MenuItemOptData,*lpMenuItemOptData;

int SaveTree(HWND hwndDlg)
{
							TVITEMA tvi;
							int count;
							char idstr[100];
							char menuItemName[256],DBString[256],MenuNameItems[256];
							char *name;
							int menuitempos,menupos;
							int MenuObjectId,runtimepos;
							PIntMenuObject pimo;

							{
								TVITEMA tvi;
								HTREEITEM hti;
								
								hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUOBJECTS));
								if (hti==NULL){return(0);};
								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
								TreeView_GetItemA(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),&tvi);
								MenuObjectId=tvi.lParam;
							}

							tvi.hItem=TreeView_GetRoot(GetDlgItem(hwndDlg,IDC_MENUITEMS));
							tvi.cchTextMax=99;
							tvi.mask=TVIF_TEXT|TVIF_PARAM|TVIF_HANDLE;
							tvi.pszText=(char *)&idstr;
							count=0;
							//lockbut();
							
							menupos=GetMenuObjbyId(MenuObjectId);
							if (menupos==-1) return(-1);
							
							pimo=&MenuObjects[menupos];

							sprintf(MenuNameItems, "%s_Items", pimo->Name); 
							runtimepos=0;

							while(tvi.hItem!=NULL) {
								//_itoa(count,buf,10);
								TreeView_GetItemA(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);
								name=((MenuItemOptData *)tvi.lParam)->name;
								menuitempos=GetMenuItembyId(menupos,((MenuItemOptData *)tvi.lParam)->id);
								if (menuitempos!=-1)
								{
									//MenuObjects[MenuObjectId].MenuItems[menuitempos].OverrideShow=
									if (pimo->MenuItems[menuitempos].UniqName)
									{
                                        sprintf(menuItemName,"{%s}",pimo->MenuItems[menuitempos].UniqName);
									}else
									{
                                        sprintf(menuItemName,"{%s}",pimo->MenuItems[menuitempos].mi.pszName);
									};



									sprintf(DBString, "%s_visible", menuItemName); 
									DBWriteContactSettingByte(NULL, MenuNameItems, DBString, ((MenuItemOptData *)tvi.lParam)->show);

									sprintf(DBString, "%s_pos", menuItemName); 								
									DBWriteContactSettingDword(NULL, MenuNameItems, DBString, runtimepos);

									if (
										MyStrCmp( ((MenuItemOptData *)tvi.lParam)->name,((MenuItemOptData *)tvi.lParam)->defname )!=0 
										)
									{
									sprintf(DBString, "%s_name", menuItemName); 
									DBWriteContactSettingString(NULL, MenuNameItems, DBString, ((MenuItemOptData *)tvi.lParam)->name);
									};
									runtimepos+=100;
								}else
								{
									
								};
								if ((MyStrCmp(name,"---------------------------------------------")==0)&&((MenuItemOptData *)tvi.lParam)->show)
								{
									runtimepos+=SEPARATORPOSITIONINTERVAL;
								};

								tvi.hItem=TreeView_GetNextSibling(GetDlgItem(hwndDlg,IDC_MENUITEMS),tvi.hItem);							
								count++;
							}

							//ulockbut();
							//ttbOptionsChanged();
							
return (1);
};


int BuildMenuObjectsTree(HWND hwndDlg)
{
			TVINSERTSTRUCTA tvis;
			int i;

			tvis.hParent=NULL;
			tvis.hInsertAfter=TVI_LAST;
			tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	
			TreeView_DeleteAllItems(GetDlgItem(hwndDlg,IDC_MENUOBJECTS));
			if (MenuObjectsCount==0){return(FALSE);};

			for(i=0;i<MenuObjectsCount;i++) {				
				tvis.item.lParam=(LPARAM)MenuObjects[i].id;
				tvis.item.pszText=Translate(MenuObjects[i].Name);
				tvis.item.iImage=tvis.item.iSelectedImage=TRUE;
				TreeView_InsertItemA(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),&tvis);			

			};
			return(1);
};

int sortfunc(const void *a,const void *b)
{
	lpMenuItemOptData *sd1,*sd2;
	sd1=(lpMenuItemOptData *)a;
	sd2=(lpMenuItemOptData *)b;
	if ((*sd1)->pos > (*sd2)->pos){return(1);};
	if ((*sd1)->pos < (*sd2)->pos){return(-1);};
	return(0);

};


int InsertSeparator(HWND hwndDlg)
{
					MenuItemOptData *PD;
					TVINSERTSTRUCTA tvis;
								TVITEMA tvi;
								HTREEITEM hti;
								
					ZeroMemory(&tvi,sizeof(tvi));
					ZeroMemory(&hti,sizeof(hti));
					hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUITEMS));
					if (hti==NULL){return 1;};
					tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_TEXT;
					tvi.hItem=hti;
					if (TreeView_GetItemA(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi)==FALSE) return 1;

					ZeroMemory(&tvis,sizeof(tvis));
					PD=(MenuItemOptData*)mir_alloc(sizeof(MenuItemOptData));
					ZeroMemory(PD,sizeof(MenuItemOptData));
					PD->id=-1;
					PD->name=mir_strdup("---------------------------------------------");
					PD->show=TRUE;
					PD->pos=((MenuItemOptData *)tvi.lParam)->pos-1;
						
					tvis.item.lParam=(LPARAM)(PD);
					tvis.item.pszText=(PD->name);
					tvis.item.iImage=tvis.item.iSelectedImage=PD->show;

					tvis.hParent=NULL;
					
					tvis.hInsertAfter=hti;
					tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	

					TreeView_InsertItemA(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvis);					
					{int err=GetLastError();
					};
					return(1);
};


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
			if (menupos==-1){return(FALSE);};
			pimo=&MenuObjects[menupos];
				
			if (pimo->MenuItemsCount==0){return(FALSE);};

			sprintf(MenuNameItems, "%s_Items", pimo->Name); 

			//*PDar
			count=0;
			for(i=0;i<pimo->MenuItemsCount;i++) {
				if (pimo->MenuItems[i].mi.root!=-1) {continue;};
			count++;
			};
			
			PDar=(lpMenuItemOptData*)mir_alloc(sizeof(lpMenuItemOptData)*count);

			count=0;
			for(i=0;i<pimo->MenuItemsCount;i++) {
				if (pimo->MenuItems[i].mi.root!=-1) {continue;};

				PD=(MenuItemOptData*)mir_alloc(sizeof(MenuItemOptData));
				ZeroMemory(PD,sizeof(MenuItemOptData));
				if (pimo->MenuItems[i].UniqName)
				{

                    sprintf(menuItemName,"{%s}",pimo->MenuItems[i].UniqName);
				}else
				{
                    sprintf(menuItemName,"{%s}",pimo->MenuItems[i].mi.pszName);
				};
				
				{
				
					DBVARIANT dbv={0};
					sprintf(buf, "%s_name", menuItemName);
					
					if (!DBGetContactSetting(NULL, MenuNameItems, buf, &dbv))
					{
						PD->name=mir_strdup(dbv.pszVal);			
					}else
					{
					PD->name=mir_strdup(pimo->MenuItems[i].mi.pszName);			
					};
          DBFreeVariant(&dbv);
				};
				
				PD->defname=mir_strdup(pimo->MenuItems[i].mi.pszName);

				sprintf(buf, "%s_visible", menuItemName);
				PD->show=DBGetContactSettingByte(NULL,MenuNameItems, buf, 1);				
			sprintf(buf, "%s_pos", menuItemName);
				PD->pos=DBGetContactSettingDword(NULL,MenuNameItems, buf, 1);
				PD->id=pimo->MenuItems[i].id;
				if (pimo->MenuItems[i].UniqName) PD->uniqname=mir_strdup(pimo->MenuItems[i].UniqName);
				
				PDar[count]=PD;
				count++;
			};
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
					PD->name=mir_strdup("---------------------------------------------");
					PD->pos=PDar[i]->pos-1;
					PD->show=TRUE;
						
					tvis.item.lParam=(LPARAM)(PD);
					tvis.item.pszText=(PD->name);
					tvis.item.iImage=tvis.item.iSelectedImage=PD->show;
					TreeView_InsertItemA(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvis);					
				};
				
				tvis.item.lParam=(LPARAM)(PDar[i]);
				tvis.item.pszText=(PDar[i]->name);
				tvis.item.iImage=tvis.item.iSelectedImage=PDar[i]->show;
				{
				HTREEITEM hti=TreeView_InsertItemA(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvis);
				if (first)
					{
					TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),hti);
					first=FALSE;
					}
				};
				

				lastpos=PDar[i]->pos;	
			};
			LockWindowUpdate(0);

			mir_free(PDar);
			
			ShowWindowNew(GetDlgItem(hwndDlg,IDC_NOTSUPPORTWARNING),(pimo->bUseUserDefinedItems)?SW_HIDE:SW_SHOW);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MENUITEMS),(pimo->bUseUserDefinedItems));
			EnableWindow(GetDlgItem(hwndDlg,IDC_INSERTSEPARATOR),(pimo->bUseUserDefinedItems));
			
return 1;
};

void RebuildCurrent(HWND hwndDlg)
{
							
								TVITEMA tvi;
								HTREEITEM hti;
								
								hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUOBJECTS));
								if (hti==NULL){return;};
								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
								TreeView_GetItemA(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),&tvi);
								BuildTree(hwndDlg,(int)tvi.lParam);
							
};

WNDPROC MyOldWindowProc=NULL;

LRESULT CALLBACK LBTNDOWNProc(HWND hwnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg==WM_LBUTTONDOWN && !(GetKeyState(VK_CONTROL)&0x8000))
    {    
	
        TVHITTESTINFO hti;
		hti.pt.x=(short)LOWORD(lParam);
	    hti.pt.y=(short)HIWORD(lParam);
//		ClientToScreen(hwndDlg,&hti.pt);
//		ScreenToClient(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti.pt);
		TreeView_HitTest(hwnd,&hti);
        if (hti.flags&TVHT_ONITEMLABEL)
        {
             /// LabelClicked Set/unset selection
            TVITEMA tvi;
            HWND tvw=hwnd;
		    tvi.mask=TVIF_HANDLE|TVIF_PARAM;
			tvi.hItem=hti.hItem;
			TreeView_GetItemA(tvw,&tvi);
            if (!((MenuItemOptData *)tvi.lParam)->isSelected) /* is not Selected*/ 
            {
                                // reset all selection except current
                HTREEITEM hit;
                hit=TreeView_GetRoot(tvw);
                if (hit)
                   do  
                   { 
                        TVITEMA tvi={0};
						tvi.mask=TVIF_HANDLE|TVIF_PARAM;
						tvi.hItem=hit;
						TreeView_GetItemA(tvw,&tvi);                                                
                        if (hti.hItem!=hit)
                            ((MenuItemOptData *)tvi.lParam)->isSelected=0;
                        else
                            ((MenuItemOptData *)tvi.lParam)->isSelected=1;
                        TreeView_SetItem(tvw,&tvi);
                   }while (hit=TreeView_GetNextSibling(tvw,hit));
            }											                                        
        }

    }
    return CallWindowProc(MyOldWindowProc,hwnd,uMsg,wParam,lParam);
}

static BOOL CALLBACK GenMenuOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{	struct OrderData *dat;

	dat=(struct OrderData*)GetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_USERDATA);

	switch (msg)
	{
		case WM_INITDIALOG: {

			TranslateDialogDefault(hwndDlg);
			dat=(struct OrderData*)mir_alloc(sizeof(struct OrderData));
			SetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_USERDATA,(long)dat);
			dat->dragging=0;
            MyOldWindowProc=(WNDPROC)GetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_WNDPROC);
	        SetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_WNDPROC,(long)&LBTNDOWNProc);

			
			//SetWindowLong(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREE),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREE),GWL_STYLE)|TVS_NOHSCROLL);

			{	HIMAGELIST himlCheckBoxes;
				himlCheckBoxes=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,2,2);
				ImageList_AddIcon(himlCheckBoxes,LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_NOTICK)));
				ImageList_AddIcon(himlCheckBoxes,LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_TICK)));
				TreeView_SetImageList(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),himlCheckBoxes,TVSIL_NORMAL);
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
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
				};
				if (ctrlid==IDC_GENMENU_DEFAULT)
				{

								TVITEMA tvi;
								HTREEITEM hti;
								char buf[256];

								hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUITEMS));
								if (hti==NULL){break;};
								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
								TreeView_GetItemA(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);
								

								if (strstr(((MenuItemOptData *)tvi.lParam)->name,"---------------------------------------------")){break;};
								ZeroMemory(buf,256);
								GetDlgItemTextA(hwndDlg,IDC_GENMENU_CUSTOMNAME,buf,256);
								if (((MenuItemOptData *)tvi.lParam)->name){mir_free(((MenuItemOptData *)tvi.lParam)->name);};
								
								((MenuItemOptData *)tvi.lParam)->name=mir_strdup( ((MenuItemOptData *)tvi.lParam)->defname );
								
								SaveTree(hwndDlg);
								RebuildCurrent(hwndDlg);
				
				};

				if (ctrlid==IDC_GENMENU_SET)
				{

								TVITEMA tvi;
								HTREEITEM hti;
								char buf[256];

								hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUITEMS));
								if (hti==NULL){break;};
								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
								TreeView_GetItemA(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);
								

								if (strstr(((MenuItemOptData *)tvi.lParam)->name,"---------------------------------------------")){break;};
								ZeroMemory(buf,256);
								GetDlgItemTextA(hwndDlg,IDC_GENMENU_CUSTOMNAME,buf,256);
								if (((MenuItemOptData *)tvi.lParam)->name){mir_free(((MenuItemOptData *)tvi.lParam)->name);};
								((MenuItemOptData *)tvi.lParam)->name=mir_strdup(buf);
								
								SaveTree(hwndDlg);
								RebuildCurrent(hwndDlg);

				
				};
				break;
			};

			if ((HIWORD(wParam)==STN_CLICKED|| HIWORD(wParam)==STN_DBLCLK))
			{
				int ctrlid=LOWORD(wParam);
			
			};
		break;
		}
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case 0: 
					switch (((LPNMHDR)lParam)->code)
					{
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
							{
								TVITEMA tvi;
								HTREEITEM hti;
								
								hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUOBJECTS));
								if (hti==NULL){break;};
								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
								TreeView_GetItemA(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),&tvi);
								BuildTree(hwndDlg,(int)tvi.lParam);
								
								
								break;
							}

					};
					break;
					};
				case IDC_MENUITEMS:
					switch (((LPNMHDR)lParam)->code) {

                        case NM_CUSTOMDRAW:
                            
                        {
                            int i= handleCustomDraw(GetDlgItem(hwndDlg,IDC_MENUITEMS),(LPNMTVCUSTOMDRAW) lParam);
                            SetWindowLong(hwndDlg, DWL_MSGRESULT, i); 
                            return TRUE;
                        }

						case TVN_BEGINDRAGA:
							SetCapture(hwndDlg);
							dat->dragging=1;
							dat->hDragItem=((LPNMTREEVIEW)lParam)->itemNew.hItem;
							TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),dat->hDragItem);
							//ShowWindowNew(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREEWARNING),SW_SHOW);
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
											TVITEMA tvi;
											tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
											tvi.hItem=hti.hItem;
											TreeView_GetItemA(((LPNMHDR)lParam)->hwndFrom,&tvi);
											tvi.iImage=tvi.iSelectedImage=!tvi.iImage;
											((MenuItemOptData *)tvi.lParam)->show=tvi.iImage;
											TreeView_SetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
											SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
											
											//all changes take effect in runtime
											//ShowWindowNew(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREEWARNING),SW_SHOW);
										}
                                        /*--------MultiSelection----------*/
                                        if (hti.flags&TVHT_ONITEMLABEL)
                                        {
                                            /// LabelClicked Set/unset selection
                                            TVITEMA tvi;
                                            HWND tvw=((LPNMHDR)lParam)->hwndFrom;
		                                    tvi.mask=TVIF_HANDLE|TVIF_PARAM;
			                                tvi.hItem=hti.hItem;
			                                TreeView_GetItemA(tvw,&tvi);
                                            if (GetKeyState(VK_CONTROL)&0x8000)
                                            {
			                                    if (((MenuItemOptData *)tvi.lParam)->isSelected)
                                                    ((MenuItemOptData *)tvi.lParam)->isSelected=0;
                                                else 
                                                ((MenuItemOptData *)tvi.lParam)->isSelected=1;  //current selection order++.
                                                TreeView_SetItem(tvw,&tvi);
                                            }
                                            else if (GetKeyState(VK_SHIFT)&0x8000)
                                            {
                                            ;  // shifted click
                                            }
                                            else 
                                            {
                                                                            // reset all selection except current
                                                HTREEITEM hit;
                                                hit=TreeView_GetRoot(tvw);
                                                if (hit)
                                                do  
                                                { 
                                                        TVITEMA tvi={0};
						                                tvi.mask=TVIF_HANDLE|TVIF_PARAM;
						                                tvi.hItem=hit;
						                                TreeView_GetItemA(tvw,&tvi);                                                
                                                        if (hti.hItem!=hit)
                                                            ((MenuItemOptData *)tvi.lParam)->isSelected=0;
                                                        else
                                                            ((MenuItemOptData *)tvi.lParam)->isSelected=1;
                                                        TreeView_SetItem(tvw,&tvi);
                                                }while (hit=TreeView_GetNextSibling(tvw,hit));
                                            }											                                        
                                        }

                                    }


							break;
                            }
                        case TVN_SELCHANGINGA:
                            {
                                LPNMTREEVIEWA pn;
                                pn = (LPNMTREEVIEWA) lParam;
                                //((MenuItemOptData *)(pn->itemNew.lParam))->isSelected=1;
                                /*if (pn->action==NotKeyPressed)
                                {
                                    remove all selection
                                }
                                */
                            }
						case TVN_SELCHANGEDA:
						
							{
							
						if(1/*TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti)*/)
							if (1/*hti.flags&TVHT_ONITEMLABEL*/)
							{

								TVITEMA tvi;
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
									break;};
								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
								TreeView_GetItemA(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);
								
								//((MenuItemOptData *)tvi.lParam)->show


								if (strstr(((MenuItemOptData *)tvi.lParam)->name,"---------------------------------------------")){break;};

								SetDlgItemTextA(hwndDlg,IDC_GENMENU_CUSTOMNAME,((MenuItemOptData *)tvi.lParam)->name);
								
								SetDlgItemTextA(hwndDlg,IDC_GENMENU_SERVICE,(((MenuItemOptData *)tvi.lParam)->uniqname)?(((MenuItemOptData *)tvi.lParam)->uniqname):"");

								EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_DEFAULT),TRUE);
								EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_SET),TRUE);
								EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_CUSTOMNAME),TRUE);

							};
							
							};
							break;


						}
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
                    HTREEITEM it=hti.hItem;
					hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_MENUITEMS))/2;
					TreeView_HitTest(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti);
                    if (!(hti.flags&TVHT_ABOVE))
					    TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_MENUITEMS),hti.hItem,1);
                    else 
                        TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_MENUITEMS),it,0);
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
			};

		case WM_LBUTTONUP:
			if(!dat->dragging) break;
			TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_MENUITEMS),NULL,0);
			dat->dragging=0;
			ReleaseCapture();
			{	TVHITTESTINFO hti;              
				hti.pt.x=(short)LOWORD(lParam);
				hti.pt.y=(short)HIWORD(lParam);
				ClientToScreen(hwndDlg,&hti.pt);
				ScreenToClient(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti.pt);
				hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_MENUITEMS))/2;
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti);
                if (hti.flags&TVHT_ABOVE) hti.hItem=TVI_FIRST;
				if(dat->hDragItem==hti.hItem) break;
                dat->hDragItem=NULL;
                if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT)||(hti.hItem==TVI_FIRST)) 
                {
                    HWND tvw;
                    HTREEITEM * pSIT;
                    HTREEITEM FirstItem=NULL;
                    UINT uITCnt,uSic ;
                    tvw=GetDlgItem(hwndDlg,IDC_MENUITEMS);
                    uITCnt=TreeView_GetCount(tvw);
                    uSic=0;
                    if (uITCnt)
                    {
                        pSIT=(HTREEITEM *)malloc(sizeof(HTREEITEM)*uITCnt);
                        if (pSIT)
                        {
                           HTREEITEM hit;
                           hit=TreeView_GetRoot(tvw);
                           if (hit)
                           do  
                            { 
                                TVITEMA tvi={0};
						        tvi.mask=TVIF_HANDLE|TVIF_PARAM;
						        tvi.hItem=hit;
						        TreeView_GetItemA(tvw,&tvi);                                                
                                if (((MenuItemOptData *)tvi.lParam)->isSelected)
                                {
                                  pSIT[uSic]=tvi.hItem;
                                  
                                  uSic++;
                                }
                            }while (hit=TreeView_GetNextSibling(tvw,hit));
                        // Proceed moving
                            {
                                UINT i;
                                HTREEITEM insertAfter;
                                insertAfter=hti.hItem;
                                for (i=0; i<uSic; i++)
                                {
                                    if (insertAfter) insertAfter=MoveItemAbove(tvw,pSIT[i],insertAfter);
                                    else break;
                                    if (!i) FirstItem=insertAfter;
                                }
                            }
                        // free pointers...
                            free(pSIT);
                        }
    				}	
                    if (FirstItem) TreeView_SelectItem(tvw,FirstItem); 
                    SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
					{
					SaveTree(hwndDlg);
					};    	
				}
			        
            }
			break; 
	}
	return FALSE;
}

HTREEITEM MoveItemAbove(HWND hTreeWnd, HTREEITEM hItem, HTREEITEM hInsertAfter)
{
    TVITEMA tvi={0};
    tvi.mask=TVIF_HANDLE|TVIF_PARAM;
    tvi.hItem=hItem;
    if (!TreeView_GetItemA(hTreeWnd,&tvi)) return NULL;
    if (hItem && hInsertAfter) 
    {   
        TVINSERTSTRUCTA tvis;
		char name[128];
        if (hItem == hInsertAfter) 
        {
            return hItem;
        }
		tvis.item.mask=TVIF_HANDLE|TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tvis.item.stateMask=0xFFFFFFFF;
		tvis.item.pszText=name;
		tvis.item.cchTextMax=sizeof(name);
		tvis.item.hItem=hItem;		
		tvis.item.iImage=tvis.item.iSelectedImage=((MenuItemOptData *)tvi.lParam)->show;
        if (!TreeView_GetItemA(hTreeWnd,&tvis.item)) return NULL;			
		if (!TreeView_DeleteItem(hTreeWnd,hItem)) return NULL;
        tvis.hParent=NULL;
		tvis.hInsertAfter=hInsertAfter;
		return TreeView_InsertItemA(hTreeWnd,&tvis);
     }
    return NULL;    
}


long handleCustomDraw(HWND hWndTreeView, LPNMTVCUSTOMDRAW pNMTVCD)
{
   if (pNMTVCD==NULL)
   {
      return -1;
   }
   //SetWindowTheme(hWndTreeView, "", "");
   switch (pNMTVCD->nmcd.dwDrawStage)
   { 
      case CDDS_PREPAINT:
         return (CDRF_NOTIFYITEMDRAW);
      case CDDS_ITEMPREPAINT:
            {

                HTREEITEM hItem = (HTREEITEM) pNMTVCD->nmcd.dwItemSpec;
                char buf[255];
                TVITEMA tvi = { 0 };
                int k=0;
                tvi.mask = TVIF_HANDLE |TVIF_PARAM|TVIS_SELECTED|TVIF_TEXT|TVIF_IMAGE;
                tvi.stateMask=TVIS_SELECTED;
                tvi.hItem = hItem;
                tvi.pszText=(LPSTR)(&buf);
                tvi.cchTextMax=254;
                TreeView_GetItemA(hWndTreeView, &tvi);
                if (((MenuItemOptData *)tvi.lParam)->isSelected)// || (tvi.state&(TVIS_DROPHILITED|TVIS_SELECTED)))
                {

                    pNMTVCD->clrTextBk = GetSysColor(COLOR_HIGHLIGHT);
                    pNMTVCD->clrText   = GetSysColor(COLOR_HIGHLIGHTTEXT);
                }
                else
                {
                    pNMTVCD->clrTextBk = GetSysColor(COLOR_WINDOW);
                    pNMTVCD->clrText   = GetSysColor(COLOR_WINDOWTEXT);
                }
    /* At this point, you can change the background colors for the item
    and any subitems and return CDRF_NEWFONT. If the list-view control
    is in report mode, you can simply return CDRF_NOTIFYSUBITEMREDRAW
    to customize the item's subitems individually */
             if (tvi.iImage==-1)
                {   
                    HBRUSH br;
                    SIZE sz; 
                    RECT rc;
                    k=1;

                    GetTextExtentPoint32A(pNMTVCD->nmcd.hdc,tvi.pszText,MyStrLen(tvi.pszText),&sz);
                    
                    if (sz.cx+3>pNMTVCD->nmcd.rc.right-pNMTVCD->nmcd.rc.left) rc=pNMTVCD->nmcd.rc;
                    else SetRect(&rc,pNMTVCD->nmcd.rc.left,pNMTVCD->nmcd.rc.top,pNMTVCD->nmcd.rc.left+sz.cx+3,pNMTVCD->nmcd.rc.bottom);

                    br=CreateSolidBrush(pNMTVCD->clrTextBk); 
                    SetTextColor(pNMTVCD->nmcd.hdc,pNMTVCD->clrText);
                    SetBkColor(pNMTVCD->nmcd.hdc,pNMTVCD->clrTextBk);
                    FillRect(pNMTVCD->nmcd.hdc,&rc,br);
                    DeleteObject(br);
                    DrawTextA(pNMTVCD->nmcd.hdc,tvi.pszText,MyStrLen(tvi.pszText),&pNMTVCD->nmcd.rc,DT_LEFT|DT_VCENTER|DT_NOPREFIX);
                }
        
                return CDRF_NEWFONT|(k?CDRF_SKIPDEFAULT:0);

            }
      case CDDS_ITEM:
          {
              int i=0;
              i++;
          }
       default:
         break;
   }
   return 0;
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
	odp.pszTitle=Translate("Menu Order");
	odp.pfnDlgProc=GenMenuOpts;
	odp.flags=ODPF_BOLDGROUPS;//|ODPF_EXPERTONLY;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	return 0;
}