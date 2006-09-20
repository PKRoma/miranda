#include "commonheaders.h"
#include "m_genmenu.h"
#include "genmenu.h"
#include "clist.h"
#pragma hdrstop 

#define STR_SEPARATOR _T("---------------------------------------------")

extern PIntMenuObject MenuObjects;
extern int MenuObjectsCount;
extern BOOL CALLBACK ProtocolOrderOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
HTREEITEM MoveItemAbove(HWND hTreeWnd, HTREEITEM hItem, HTREEITEM hInsertAfter);
long handleCustomDraw(HWND hWndTreeView, LPNMTVCUSTOMDRAW pNMTVCD);

int hInst;

struct OrderData
{
	int dragging;
	HTREEITEM hDragItem;
};

typedef struct tagMenuItemOptData
{
	TCHAR *name;
	TCHAR *defname;
		char *uniqname;

        int pos;
        boolean show;
        DWORD isSelected;
		int id;
}
	MenuItemOptData,*lpMenuItemOptData;

int SaveTree(HWND hwndDlg)
{
	TVITEM tvi;
							int count;
	TCHAR idstr[100];
							char menuItemName[256],DBString[256],MenuNameItems[256];
	TCHAR *name;
							int menuitempos,menupos;
							int MenuObjectId,runtimepos;
							PIntMenuObject pimo;

							{
		TVITEM tvi;
		HTREEITEM hti = TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUOBJECTS));
		if ( hti == NULL )
			return(0);
								
								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
		SendDlgItemMessage(hwndDlg, IDC_MENUOBJECTS, TVM_GETITEM, 0, (LPARAM)&tvi);
								MenuObjectId=tvi.lParam;
							}

							tvi.hItem=TreeView_GetRoot(GetDlgItem(hwndDlg,IDC_MENUITEMS));
							tvi.cchTextMax=99;
							tvi.mask=TVIF_TEXT|TVIF_PARAM|TVIF_HANDLE;
							tvi.pszText = idstr;
							count=0;
							//lockbut();
							
							menupos=GetMenuObjbyId(MenuObjectId);
	if ( menupos == -1 )
		return(-1);
							
							pimo=&MenuObjects[menupos];

	wsprintfA(MenuNameItems, "%s_Items", pimo->Name); 
							runtimepos=0;

							while(tvi.hItem!=NULL) {
		TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);
								name=((MenuItemOptData *)tvi.lParam)->name;
								menuitempos=GetMenuItembyId(menupos,((MenuItemOptData *)tvi.lParam)->id);
		if (menuitempos!=-1) {

			GetMenuItemName( &pimo->MenuItems[menuitempos], menuItemName, sizeof(menuItemName));

			wsprintfA(DBString, "%s_visible", menuItemName); 
									DBWriteContactSettingByte(NULL, MenuNameItems, DBString, ((MenuItemOptData *)tvi.lParam)->show);

			wsprintfA(DBString, "%s_pos", menuItemName);                                 
									DBWriteContactSettingDword(NULL, MenuNameItems, DBString, runtimepos);

			if ( _tcscmp( ((MenuItemOptData *)tvi.lParam)->name,((MenuItemOptData *)tvi.lParam)->defname )!=0 ) {
				wsprintfA(DBString, "%s_name", menuItemName); 
				DBWriteContactSettingTString(NULL, MenuNameItems, DBString, ((MenuItemOptData *)tvi.lParam)->name);
			}
									runtimepos+=100;
		}
									
		if ( !_tcscmp(name, STR_SEPARATOR) && ((MenuItemOptData *)tvi.lParam)->show )
									runtimepos+=SEPARATORPOSITIONINTERVAL;

								tvi.hItem=TreeView_GetNextSibling(GetDlgItem(hwndDlg,IDC_MENUITEMS),tvi.hItem);							
								count++;
							}
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
	if (MenuObjectsCount==0)
		return(FALSE);

			for(i=0;i<MenuObjectsCount;i++) {				
				tvis.item.lParam=(LPARAM)MenuObjects[i].id;
				tvis.item.pszText=Translate(MenuObjects[i].Name);
				tvis.item.iImage=tvis.item.iSelectedImage=TRUE;
		SendDlgItemMessageA(hwndDlg, IDC_MENUOBJECTS, TVM_INSERTITEMA, 0, (LPARAM)&tvis);
	}
			return(1);
}

int sortfunc(const void *a,const void *b)
{
	lpMenuItemOptData *sd1,*sd2;
	sd1=(lpMenuItemOptData *)a;
	sd2=(lpMenuItemOptData *)b;
	if ((*sd1)->pos > (*sd2)->pos)
		return(1);

	if ((*sd1)->pos < (*sd2)->pos)
		return(-1);

	return(0);
}

int InsertSeparator(HWND hwndDlg)
{
					MenuItemOptData *PD;
	TVINSERTSTRUCT tvis;
	TVITEM tvi;
								HTREEITEM hti;
								
					ZeroMemory(&tvi,sizeof(tvi));
					ZeroMemory(&hti,sizeof(hti));
					hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUITEMS));
	if (hti==NULL)
		return 1;

					tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_TEXT;
					tvi.hItem=hti;
	if (TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi)==FALSE) 
		return 1;

					ZeroMemory(&tvis,sizeof(tvis));
					PD=(MenuItemOptData*)mir_alloc(sizeof(MenuItemOptData));
					ZeroMemory(PD,sizeof(MenuItemOptData));
					PD->id=-1;
	PD->name = mir_tstrdup( STR_SEPARATOR );
					PD->show=TRUE;
					PD->pos=((MenuItemOptData *)tvi.lParam)->pos-1;
						
					tvis.item.lParam=(LPARAM)(PD);
					tvis.item.pszText=(PD->name);
					tvis.item.iImage=tvis.item.iSelectedImage=PD->show;

					tvis.hParent=NULL;
					
					tvis.hInsertAfter=hti;
					tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	
	SendDlgItemMessage(hwndDlg, IDC_MENUITEMS, TVM_INSERTITEM, 0, (LPARAM)&tvis);
	return(1);
}

void FreeTreeData( HWND hwndDlg )
{
	HTREEITEM hItem = TreeView_GetRoot(GetDlgItem(hwndDlg,IDC_MENUITEMS));
	while ( hItem != NULL ) {
		TVITEM tvi;
		tvi.mask = TVIF_HANDLE|TVIF_PARAM;
		tvi.hItem = hItem;
		TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS), &tvi);
		{	MenuItemOptData* O = (MenuItemOptData *)tvi.lParam;
			if ( O->name ) mir_free( O->name );
			if ( O->defname ) mir_free( O->defname );
			if ( O->uniqname ) mir_free( O->uniqname );
			mir_free( O );
		}

		tvi.lParam = 0;
		TreeView_SetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS), &tvi);

		hItem = TreeView_GetNextSibling(GetDlgItem(hwndDlg,IDC_MENUITEMS), hItem);
}	}

int BuildTree(HWND hwndDlg,int MenuObjectId)
{
				
			struct OrderData *dat;
	TVINSERTSTRUCT tvis;
			MenuItemOptData *PD;
			lpMenuItemOptData *PDar;
			boolean first;
			
				
			char menuItemName[256],MenuNameItems[256];
			char buf[256];
			int i,count,menupos,lastpos;
			PIntMenuObject pimo;
			
	FreeTreeData(hwndDlg);

			dat=(struct OrderData*)GetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_USERDATA);
			tvis.hParent=NULL;
			tvis.hInsertAfter=TVI_LAST;
			tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	
			TreeView_DeleteAllItems(GetDlgItem(hwndDlg,IDC_MENUITEMS));

	menupos=GetMenuObjbyId(MenuObjectId);
	if (menupos==-1)
		return(FALSE);

			pimo=&MenuObjects[menupos];
				
	if (pimo->MenuItemsCount==0)
		return(FALSE);

	wsprintfA(MenuNameItems, "%s_Items", pimo->Name); 

			//*PDar
			count=0;
			for(i=0;i<pimo->MenuItemsCount;i++) {
		if (pimo->MenuItems[i].mi.root!=-1)
			continue;

			count++;
	}
			
	PDar=mir_alloc(sizeof(lpMenuItemOptData)*count);

			count=0;
			for(i=0;i<pimo->MenuItemsCount;i++) {
		if (pimo->MenuItems[i].mi.root!=-1)
			continue;

				PD=(MenuItemOptData*)mir_alloc(sizeof(MenuItemOptData));
				ZeroMemory(PD,sizeof(MenuItemOptData));
		GetMenuItemName( &pimo->MenuItems[i], menuItemName, sizeof(menuItemName));
				{
			DBVARIANT dbv;
			wsprintfA(buf, "%s_name", menuItemName);

			if ( !DBGetContactSettingTString(NULL, MenuNameItems, buf, &dbv)) {
				PD->name = mir_tstrdup(dbv.ptszVal);            
				//mir_free(dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			else PD->name = mir_tstrdup(pimo->MenuItems[i].mi.ptszName);
		}

		PD->defname=mir_tstrdup(pimo->MenuItems[i].mi.ptszName);
				
		wsprintfA(buf, "%s_visible", menuItemName);
		PD->show=DBGetContactSettingByte(NULL,MenuNameItems, buf, 1);           
				
		wsprintfA(buf, "%s_pos", menuItemName);
		PD->pos=DBGetContactSettingDword(NULL,MenuNameItems, buf, 1);
					
		PD->id=pimo->MenuItems[i].id;
					{
			char buf[256];
			wsprintfA(buf,"buildtree name: %s,id: %d\r\n",pimo->MenuItems[i].mi.pszName,pimo->MenuItems[i].id);
		}
				
				if (pimo->MenuItems[i].UniqName) PD->uniqname=mir_strdup(pimo->MenuItems[i].UniqName);
				
				PDar[count]=PD;
				count++;
	}
			qsort(&(PDar[0]),count,sizeof(lpMenuItemOptData),sortfunc);

	SendDlgItemMessage(hwndDlg, IDC_MENUITEMS, WM_SETREDRAW, FALSE, 0);
			lastpos=0;
			first=TRUE;
			for(i=0;i<count;i++) {

		if (PDar[i]->pos-lastpos>=SEPARATORPOSITIONINTERVAL) {
					PD=(MenuItemOptData*)mir_alloc(sizeof(MenuItemOptData));
					ZeroMemory(PD,sizeof(MenuItemOptData));
					PD->id=-1;
			PD->name=mir_tstrdup( _T("---------------------------------------------"));
					PD->pos=PDar[i]->pos-1;
					PD->show=TRUE;
						
					tvis.item.lParam=(LPARAM)(PD);
					tvis.item.pszText=(PD->name);
					tvis.item.iImage=tvis.item.iSelectedImage=PD->show;
			SendDlgItemMessage(hwndDlg, IDC_MENUITEMS, TVM_INSERTITEM, 0, (LPARAM)&tvis);
		}
				
				tvis.item.lParam=(LPARAM)(PDar[i]);
				tvis.item.pszText=(PDar[i]->name);
				tvis.item.iImage=tvis.item.iSelectedImage=PDar[i]->show;
				{
			HTREEITEM hti = (HTREEITEM)SendDlgItemMessage(hwndDlg, IDC_MENUITEMS, TVM_INSERTITEM, 0, (LPARAM)&tvis);
			if (first) {
					TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),hti);
					first=FALSE;
		}	}

				lastpos=PDar[i]->pos;	
	}

	SendDlgItemMessage(hwndDlg, IDC_MENUITEMS, WM_SETREDRAW, TRUE, 0);
			mir_free(PDar);
	ShowWindow(GetDlgItem(hwndDlg,IDC_NOTSUPPORTWARNING),(pimo->bUseUserDefinedItems)?SW_HIDE:SW_SHOW);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MENUITEMS),(pimo->bUseUserDefinedItems));
			EnableWindow(GetDlgItem(hwndDlg,IDC_INSERTSEPARATOR),(pimo->bUseUserDefinedItems));
			
return 1;
}

void RebuildCurrent(HWND hwndDlg)
{
	TVITEM tvi;
								HTREEITEM hti;
								
								hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUOBJECTS));
	if (hti==NULL)
		return;

								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
	TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),&tvi);
								BuildTree(hwndDlg,(int)tvi.lParam);
}

WNDPROC MyOldWindowProc=NULL;

LRESULT CALLBACK LBTNDOWNProc(HWND hwnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg==WM_LBUTTONDOWN && !(GetKeyState(VK_CONTROL)&0x8000)) {
	
        TVHITTESTINFO hti;
		hti.pt.x=(short)LOWORD(lParam);
	    hti.pt.y=(short)HIWORD(lParam);
//		ClientToScreen(hwndDlg,&hti.pt);
//		ScreenToClient(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti.pt);
		TreeView_HitTest(hwnd,&hti);
		if (hti.flags&TVHT_ONITEMLABEL) {
             /// LabelClicked Set/unset selection
			TVITEM tvi;
            HWND tvw=hwnd;
		    tvi.mask=TVIF_HANDLE|TVIF_PARAM;
			tvi.hItem=hti.hItem;
			TreeView_GetItem( tvw, &tvi );

			if (!((MenuItemOptData *)tvi.lParam)->isSelected) { /* is not Selected*/
                                // reset all selection except current
                HTREEITEM hit;
                hit=TreeView_GetRoot(tvw);
                if (hit)
					do {
						TVITEM tvi={0};
						tvi.mask=TVIF_HANDLE|TVIF_PARAM;
						tvi.hItem=hit;
						TreeView_GetItem(tvw, &tvi);

                        if (hti.hItem!=hit)
                            ((MenuItemOptData *)tvi.lParam)->isSelected=0;
                        else
                            ((MenuItemOptData *)tvi.lParam)->isSelected=1;
                        TreeView_SetItem(tvw,&tvi);
        }
						while (hit=TreeView_GetNextSibling(tvw,hit));
	}	}	}

    return CallWindowProc(MyOldWindowProc,hwnd,uMsg,wParam,lParam);
}

static BOOL CALLBACK GenMenuOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct OrderData *dat = (struct OrderData*)GetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_USERDATA);

	switch (msg) {
		case WM_INITDIALOG: {

			TranslateDialogDefault(hwndDlg);
			dat=(struct OrderData*)mir_alloc(sizeof(struct OrderData));
		SetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_USERDATA,(LONG)dat);
			dat->dragging=0;
            MyOldWindowProc=(WNDPROC)GetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_WNDPROC);
		SetWindowLong(GetDlgItem(hwndDlg,IDC_MENUITEMS),GWL_WNDPROC,(LONG)&LBTNDOWNProc);

			
			//SetWindowLong(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREE),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREE),GWL_STYLE)|TVS_NOHSCROLL);

		{   
			HIMAGELIST himlCheckBoxes;
				himlCheckBoxes=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,2,2);
				ImageList_AddIcon(himlCheckBoxes,LoadSmallIconShared(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_NOTICK)));
				ImageList_AddIcon(himlCheckBoxes,LoadSmallIconShared(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_TICK)));
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
			if ((HIWORD(wParam)==BN_CLICKED|| HIWORD(wParam)==BN_DBLCLK)) {
				int ctrlid=LOWORD(wParam);
				if (ctrlid==IDC_INSERTSEPARATOR) {
					InsertSeparator(hwndDlg);
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				}
				if (ctrlid==IDC_GENMENU_DEFAULT) {

					TVITEM tvi;
								HTREEITEM hti;
								char buf[256];

								hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUITEMS));
					if (hti==NULL)
						break;

								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
					TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);
								
					if ( _tcsstr(((MenuItemOptData *)tvi.lParam)->name, _T("---------------------------------------------")))
						break;

								ZeroMemory(buf,256);
								GetDlgItemTextA(hwndDlg,IDC_GENMENU_CUSTOMNAME,buf,256);
					if (((MenuItemOptData *)tvi.lParam)->name) {
						mir_free(((MenuItemOptData *)tvi.lParam)->name);
					}
								
					((MenuItemOptData *)tvi.lParam)->name = mir_tstrdup( ((MenuItemOptData *)tvi.lParam)->defname );
								
								SaveTree(hwndDlg);
								RebuildCurrent(hwndDlg);
				
				}

				if (ctrlid==IDC_GENMENU_SET) {

					TVITEM tvi;
					TCHAR buf[256];

					HTREEITEM hti = TreeView_GetSelection( GetDlgItem( hwndDlg,IDC_MENUITEMS ));
					if ( hti == NULL )
						break;

								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
					SendDlgItemMessage(hwndDlg, IDC_MENUITEMS, TVM_GETITEM, 0, (LPARAM)&tvi);

					if ( _tcsstr(((MenuItemOptData *)tvi.lParam)->name, STR_SEPARATOR ))
						break;
								
					ZeroMemory(buf,sizeof( buf ));
					GetDlgItemText( hwndDlg, IDC_GENMENU_CUSTOMNAME, buf, SIZEOF( buf ));
					if (((MenuItemOptData *)tvi.lParam)->name)
						mir_free(((MenuItemOptData *)tvi.lParam)->name);

					((MenuItemOptData *)tvi.lParam)->name = mir_tstrdup(buf);
								
								SaveTree(hwndDlg);
								RebuildCurrent(hwndDlg);
				}
				break;
			}

			if ((HIWORD(wParam)==STN_CLICKED|| HIWORD(wParam)==STN_DBLCLK)) {
				int ctrlid=LOWORD(wParam);
			
			}
		break;
		}
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case 0: 
			if (((LPNMHDR)lParam)->code == PSN_APPLY ) {   
							SaveTree(hwndDlg);
							RebuildCurrent(hwndDlg);
						}
					break;

				case IDC_MENUOBJECTS:	
			if (((LPNMHDR)lParam)->code == TVN_SELCHANGEDA )
					{
				TVITEM tvi;
								HTREEITEM hti;
								
								hti=TreeView_GetSelection(GetDlgItem(hwndDlg,IDC_MENUOBJECTS));
				if (hti==NULL)
					break;
								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
				TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUOBJECTS),&tvi);
								BuildTree(hwndDlg,(int)tvi.lParam);
			}
								break;

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
							//CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREEWARNING),SW_SHOW);
							break;
						
						case NM_CLICK:
							{
									TVHITTESTINFO hti;
									hti.pt.x=(short)LOWORD(GetMessagePos());
									hti.pt.y=(short)HIWORD(GetMessagePos());
									ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
					if (TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti)) {
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
											//CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_BUTTONORDERTREEWARNING),SW_SHOW);
										}
                                        /*--------MultiSelection----------*/
						if (hti.flags&TVHT_ONITEMLABEL) {
                                            /// LabelClicked Set/unset selection
							TVITEM tvi;
                                            HWND tvw=((LPNMHDR)lParam)->hwndFrom;
		                                    tvi.mask=TVIF_HANDLE|TVIF_PARAM;
			                                tvi.hItem=hti.hItem;
							TreeView_GetItem(tvw,&tvi);
							if (GetKeyState(VK_CONTROL)&0x8000) {
			                                    if (((MenuItemOptData *)tvi.lParam)->isSelected)
                                                    ((MenuItemOptData *)tvi.lParam)->isSelected=0;
                                                else 
                                                ((MenuItemOptData *)tvi.lParam)->isSelected=1;  //current selection order++.
                                                TreeView_SetItem(tvw,&tvi);
                                            }
							else if (GetKeyState(VK_SHIFT)&0x8000) {
                                            ;  // shifted click
                                            }
							else {
                                                                            // reset all selection except current
                                                HTREEITEM hit;
                                                hit=TreeView_GetRoot(tvw);
                                                if (hit)
									do {
										TVITEM tvi={0};
						                                tvi.mask=TVIF_HANDLE|TVIF_PARAM;
						                                tvi.hItem=hit;
										TreeView_GetItem(tvw,&tvi);

                                                        if (hti.hItem!=hit)
                                                            ((MenuItemOptData *)tvi.lParam)->isSelected=0;
                                                        else
                                                            ((MenuItemOptData *)tvi.lParam)->isSelected=1;
                                                        TreeView_SetItem(tvw,&tvi);
                                            }											                                        
										while (hit=TreeView_GetNextSibling(tvw,hit));
					}	}	}
							break;
                            }
			case TVN_SELCHANGING:
                            {
					LPNMTREEVIEW pn;
					pn = (LPNMTREEVIEW) lParam;
                                //((MenuItemOptData *)(pn->itemNew.lParam))->isSelected=1;
                                /*if (pn->action==NotKeyPressed)
                                {
                                    remove all selection
                                }
                                */
                            }
						case TVN_SELCHANGEDA:
						
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
						break;
					}
								tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
								tvi.hItem=hti;
					TreeView_GetItem(GetDlgItem(hwndDlg,IDC_MENUITEMS),&tvi);
								//((MenuItemOptData *)tvi.lParam)->show
					if ( tvi.lParam == 0 )
						break;

					if ( _tcsstr(((MenuItemOptData *)tvi.lParam)->name, STR_SEPARATOR ))
						break;

					SetDlgItemText(hwndDlg,IDC_GENMENU_CUSTOMNAME,((MenuItemOptData *)tvi.lParam)->name);
								
								SetDlgItemTextA(hwndDlg,IDC_GENMENU_SERVICE,(((MenuItemOptData *)tvi.lParam)->uniqname)?(((MenuItemOptData *)tvi.lParam)->uniqname):"");

								EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_DEFAULT),TRUE);
								EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_SET),TRUE);
								EnableWindow(GetDlgItem(hwndDlg,IDC_GENMENU_CUSTOMNAME),TRUE);
							break;


						}
					break;
		}	}
			break;
   
    case WM_MOUSEMOVE:
			
			if(!dat||!dat->dragging) break;
		{   
			TVHITTESTINFO hti;
                
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
		}	}
		break;
				
		case WM_DESTROY:
			{
		struct OrderData* dat = (struct OrderData*)GetWindowLong( GetDlgItem(hwndDlg,IDC_MENUITEMS), GWL_USERDATA );
		if ( dat )
			mir_free( dat );
//				OptionsOpened=FALSE;
		FreeTreeData( hwndDlg );
		break;
	}

		case WM_LBUTTONUP:
		if (!dat->dragging)
			break;

			TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_MENUITEMS),NULL,0);
			dat->dragging=0;
			ReleaseCapture();
		{   
			TVHITTESTINFO hti;
				hti.pt.x=(short)LOWORD(lParam);
				hti.pt.y=(short)HIWORD(lParam);
				ClientToScreen(hwndDlg,&hti.pt);
				ScreenToClient(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti.pt);
				hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_MENUITEMS))/2;
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_MENUITEMS),&hti);
                if (hti.flags&TVHT_ABOVE) hti.hItem=TVI_FIRST;
				if(dat->hDragItem==hti.hItem) break;
                dat->hDragItem=NULL;
			if (hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT)||(hti.hItem==TVI_FIRST)) {
                    HWND tvw;
                    HTREEITEM * pSIT;
                    HTREEITEM FirstItem=NULL;
                    UINT uITCnt,uSic ;
                    tvw=GetDlgItem(hwndDlg,IDC_MENUITEMS);
                    uITCnt=TreeView_GetCount(tvw);
                    uSic=0;
				if (uITCnt) {
                        pSIT=(HTREEITEM *)malloc(sizeof(HTREEITEM)*uITCnt);
					if (pSIT) {
                           HTREEITEM hit;
                           hit=TreeView_GetRoot(tvw);
                           if (hit)
							do {
								TVITEM tvi={0};
						        tvi.mask=TVIF_HANDLE|TVIF_PARAM;
						        tvi.hItem=hit;
								TreeView_GetItem(tvw,&tvi);                                                
								if (((MenuItemOptData *)tvi.lParam)->isSelected) {
                                  pSIT[uSic]=tvi.hItem;
                                  
                                  uSic++;
                                }
                            }while (hit=TreeView_GetNextSibling(tvw,hit));
                        // Proceed moving
                            {
                                UINT i;
                                HTREEITEM insertAfter;
                                insertAfter=hti.hItem;
							for (i=0; i<uSic; i++) {
                                    if (insertAfter) insertAfter=MoveItemAbove(tvw,pSIT[i],insertAfter);
                                    else break;
                                    if (!i) FirstItem=insertAfter;
						}	}
                        // free pointers...
                            free(pSIT);
				}	}

                    if (FirstItem) TreeView_SelectItem(tvw,FirstItem); 
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					SaveTree(hwndDlg);
		}	}
			break; 
	}
	return FALSE;
}

HTREEITEM MoveItemAbove(HWND hTreeWnd, HTREEITEM hItem, HTREEITEM hInsertAfter)
{
	TVITEM tvi={0};
    tvi.mask=TVIF_HANDLE|TVIF_PARAM;
    tvi.hItem=hItem;
	if (!SendMessage(hTreeWnd, TVM_GETITEM, 0, (LPARAM)&tvi))
		return NULL;
	if (hItem && hInsertAfter) {
		TVINSERTSTRUCT tvis;
		TCHAR name[128];
        if (hItem == hInsertAfter) 
            return hItem;

		tvis.item.mask=TVIF_HANDLE|TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tvis.item.stateMask=0xFFFFFFFF;
		tvis.item.pszText=name;
		tvis.item.cchTextMax=sizeof(name);
		tvis.item.hItem=hItem;		
		tvis.item.iImage=tvis.item.iSelectedImage=((MenuItemOptData *)tvi.lParam)->show;
		if(!SendMessage(hTreeWnd, TVM_GETITEM, 0, (LPARAM)&tvis.item))
			return NULL;
		if (!TreeView_DeleteItem(hTreeWnd,hItem)) 
			return NULL;
        tvis.hParent=NULL;
		tvis.hInsertAfter=hInsertAfter;
		return TreeView_InsertItem(hTreeWnd, &tvis);
     }
    return NULL;    
}


long handleCustomDraw(HWND hWndTreeView, LPNMTVCUSTOMDRAW pNMTVCD)
{
   if (pNMTVCD==NULL)
      return -1;

   //SetWindowTheme(hWndTreeView, "", "");
	switch (pNMTVCD->nmcd.dwDrawStage) {
      case CDDS_PREPAINT:
         return (CDRF_NOTIFYITEMDRAW);
      case CDDS_ITEMPREPAINT:
            {

                HTREEITEM hItem = (HTREEITEM) pNMTVCD->nmcd.dwItemSpec;
			TCHAR buf[255];
			TVITEM tvi = {0};
                int k=0;
                tvi.mask = TVIF_HANDLE |TVIF_PARAM|TVIS_SELECTED|TVIF_TEXT|TVIF_IMAGE;
                tvi.stateMask=TVIS_SELECTED;
                tvi.hItem = hItem;
			tvi.pszText=(LPTSTR)(&buf);
                tvi.cchTextMax=254;
			TreeView_GetItem(hWndTreeView, &tvi);
			if (((MenuItemOptData *)tvi.lParam)->isSelected) {// || (tvi.state&(TVIS_DROPHILITED|TVIS_SELECTED)))

                    pNMTVCD->clrTextBk = GetSysColor(COLOR_HIGHLIGHT);
                    pNMTVCD->clrText   = GetSysColor(COLOR_HIGHLIGHTTEXT);
                }
			else {
                    pNMTVCD->clrTextBk = GetSysColor(COLOR_WINDOW);
                    pNMTVCD->clrText   = GetSysColor(COLOR_WINDOWTEXT);
                }
    /* At this point, you can change the background colors for the item
    and any subitems and return CDRF_NEWFONT. If the list-view control
    is in report mode, you can simply return CDRF_NOTIFYSUBITEMREDRAW
    to customize the item's subitems individually */
			if (tvi.iImage==-1) {
                    HBRUSH br;
                    SIZE sz; 
                    RECT rc;
                    k=1;

				GetTextExtentPoint32(pNMTVCD->nmcd.hdc,tvi.pszText,lstrlen(tvi.pszText),&sz);
                    
                    if (sz.cx+3>pNMTVCD->nmcd.rc.right-pNMTVCD->nmcd.rc.left) rc=pNMTVCD->nmcd.rc;
                    else SetRect(&rc,pNMTVCD->nmcd.rc.left,pNMTVCD->nmcd.rc.top,pNMTVCD->nmcd.rc.left+sz.cx+3,pNMTVCD->nmcd.rc.bottom);

                    br=CreateSolidBrush(pNMTVCD->clrTextBk); 
                    SetTextColor(pNMTVCD->nmcd.hdc,pNMTVCD->clrText);
                    SetBkColor(pNMTVCD->nmcd.hdc,pNMTVCD->clrTextBk);
                    FillRect(pNMTVCD->nmcd.hdc,&rc,br);
                    DeleteObject(br);
				DrawText(pNMTVCD->nmcd.hdc,tvi.pszText,lstrlen(tvi.pszText),&pNMTVCD->nmcd.rc,DT_LEFT|DT_VCENTER|DT_NOPREFIX);
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
    if (MirandaExiting()) return 0;
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