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
#include "commonprototypes.h"
#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4
extern int SkinEditorOptInit(WPARAM wParam,LPARAM lParam);
extern BOOL (WINAPI *MyUpdateLayeredWindow)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);
HWND hCLUIwnd=NULL;
//LOGFONTA LoadLogFontFromDB(char * section, char * id, DWORD * color);
extern HMENU hMenuMain;
extern BOOL IsOnDesktop;
extern BOOL (WINAPI *MySetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
//extern hFrameHelperStatusBar;
extern void ReAssignExtraIcons();
extern int CluiProtocolStatusChanged(WPARAM wParam,LPARAM lParam);
extern int UseOwnerDrawStatusBar;
//extern int OnStatusBarBackgroundChange();
extern BOOL SmoothAnimation;
extern int SetParentForContainers(HWND parent);
static UINT expertOnlyControls[]={IDC_BRINGTOFRONT, IDC_AUTOSIZE,IDC_STATIC21,IDC_MAXSIZEHEIGHT,IDC_MAXSIZESPIN,IDC_STATIC22,IDC_AUTOSIZEUPWARD,IDC_SHOWMAINMENU,IDC_SHOWCAPTION,IDC_CLIENTDRAG};
extern BYTE UseKeyColor;
extern DWORD KeyColor;
extern BOOL IsInChangingMode;
extern void ChangeWindowMode();

int CluiOptInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=0;
	odp.hInstance=g_hInst;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_CLUI);
	odp.pszTitle=Translate("Window");
	odp.pszGroup=Translate("Contact List");
	odp.pfnDlgProc=DlgProcCluiOpts;
	odp.flags=ODPF_BOLDGROUPS;
	odp.nIDBottomSimpleControl=IDC_STWINDOWGROUP;
	odp.expertOnlyControls=expertOnlyControls;
	odp.nExpertOnlyControls=sizeof(expertOnlyControls)/sizeof(expertOnlyControls[0]);
	//CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_SBAR);
	odp.pszTitle=Translate("Status Bar");
	odp.pfnDlgProc=DlgProcSBarOpts;
	odp.flags=ODPF_BOLDGROUPS|ODPF_EXPERTONLY;
	odp.nIDBottomSimpleControl=0;
	odp.nExpertOnlyControls=0;
	odp.expertOnlyControls=NULL;
	//CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	
	return 0;
}

typedef struct _OrderTreeData 
{ 
	BYTE	ID;
	TCHAR *	Name;
	BYTE	Position;
	char *  KeyName;
	BOOL	Visible;
} *pOrderTreeData, TOrderTreeData;

TOrderTreeData OrderTreeData[]=
{
	{EXTRA_ICON_EMAIL, _T("E-mail"), EXTRA_ICON_EMAIL, "EXTRA_ICON_EMAIL", TRUE},
	{EXTRA_ICON_PROTO, _T("Protocol"), EXTRA_ICON_PROTO, "EXTRA_ICON_PROTO", TRUE},
	{EXTRA_ICON_SMS, _T("Phone/SMS"), EXTRA_ICON_SMS,	 "EXTRA_ICON_SMS", TRUE},
	{EXTRA_ICON_WEB, _T("Web page"), EXTRA_ICON_WEB,   "EXTRA_ICON_WEB", TRUE},
	{EXTRA_ICON_CLIENT, _T("Client (fingerprint.dll is required)"), EXTRA_ICON_CLIENT,"EXTRA_ICON_CLIENT", TRUE},
	{EXTRA_ICON_ADV1, _T("Advanced #1"), EXTRA_ICON_ADV1,  "EXTRA_ICON_ADV1", TRUE},
	{EXTRA_ICON_ADV2, _T("Advanced #2"), EXTRA_ICON_ADV2,  "EXTRA_ICON_ADV2", TRUE},
	{EXTRA_ICON_ADV3, _T("Advanced #3"), EXTRA_ICON_ADV3,  "EXTRA_ICON_ADV3", TRUE},
	{EXTRA_ICON_ADV4, _T("Advanced #4"), EXTRA_ICON_ADV4,	"EXTRA_ICON_ADV4", TRUE}
};

#define PrVer 3

char **settingname;
int arrlen;

static int OrderEnumProc (const char *szSetting,LPARAM lParam)
{

	if (szSetting==NULL) return 0;
	if (!WildCompare((char*) szSetting,(char *) lParam,0)) return 0;
	arrlen++;
	settingname=(char **)realloc(settingname,arrlen*sizeof(char *));
	settingname[arrlen-1]=_strdup(szSetting);
	return 0;
};

int  DeleteAllSettingInOrder()
{
	DBCONTACTENUMSETTINGS dbces;
	arrlen=0;

	dbces.pfnEnumProc=OrderEnumProc;
	dbces.szModule=CLUIFrameModule;
	dbces.ofsSettings=0;
	dbces.lParam=(LPARAM)"ORDER_EXTRA_*";

	CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);

	//delete all settings
	if (arrlen==0){return(0);};
	{
		int i;
		for (i=0;i<arrlen;i++)
		{
			DBDeleteContactSetting(0,CLUIFrameModule,settingname[i]);
			free(settingname[i]);
		};
		free(settingname);
		settingname=NULL;
	};
	return(0);
};
int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	pOrderTreeData A=(pOrderTreeData)lParam1;
	pOrderTreeData B=(pOrderTreeData)lParam2;
	return (int)(A->Position)-(int)(B->Position);
}

int FillOrderTree(HWND hwndDlg, HWND Tree)
{
	int i=0;
	TVINSERTSTRUCT tvis={0};
	TreeView_DeleteAllItems(Tree);
	tvis.hParent=NULL;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	
	if (DBGetContactSettingByte(NULL,CLUIFrameModule,"ExtraCountStored",0)!=SIZEOF(OrderTreeData))
	{
		DeleteAllSettingInOrder();
		DBWriteContactSettingByte(NULL,CLUIFrameModule,"ExtraCountStored",SIZEOF(OrderTreeData));
	}
	for (i=0; i<SIZEOF(OrderTreeData); i++)
	{
		char buf[256];
		sprintf(buf,"ORDER_%s",OrderTreeData[i].KeyName);
		OrderTreeData[i].Position=(BYTE)DBGetContactSettingByte(NULL,CLUIFrameModule,buf,i);
		OrderTreeData[i].Visible =(BOOL)DBGetContactSettingByte(NULL,CLUIFrameModule,OrderTreeData[i].KeyName,1);
		tvis.hInsertAfter=TVI_LAST;
		tvis.item.lParam=(LPARAM)(&(OrderTreeData[i]));
		tvis.item.pszText=TranslateTS(OrderTreeData[i].Name);
		tvis.item.iImage=tvis.item.iSelectedImage=OrderTreeData[i].Visible;
		TreeView_InsertItem(Tree,&tvis);
	}
	{
		TVSORTCB sort={0};
		sort.hParent=NULL;
		sort.lParam=0;
		sort.lpfnCompare=CompareFunc;
		TreeView_SortChildrenCB(Tree,&sort,0);
	}
	return 0;
};
int SaveOrderTree(HWND hwndDlg, HWND Tree)
{
	HTREEITEM ht;
	BYTE pos=0;
	TVITEMA tvi={0};
	tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	ht=TreeView_GetRoot(Tree);
    do
	{
		TOrderTreeData * it=NULL;
		tvi.hItem=ht;
		TreeView_GetItemA(Tree,&tvi);
		it=(TOrderTreeData*)(tvi.lParam);
		{
			char buf[250];
			sprintf(buf,"ORDER_%s",it->KeyName);
			DBWriteContactSettingByte(NULL,CLUIFrameModule,buf,pos);
			DBWriteContactSettingByte(NULL,CLUIFrameModule,it->KeyName, it->Visible);	
		}
		ht=TreeView_GetNextSibling(Tree,ht);
		pos++;
	}while (ht);

	return 0;
}

int LoadPositionsFromDB(BYTE * OrderPos)
{
	int i=0;
	if (DBGetContactSettingByte(NULL,CLUIFrameModule,"ExtraCountStored",0)!=SIZEOF(OrderTreeData))
	{
		DeleteAllSettingInOrder();
		DBWriteContactSettingByte(NULL,CLUIFrameModule,"ExtraCountStored",SIZEOF(OrderTreeData));
	}

	for (i=0; i<SIZEOF(OrderTreeData); i++)
	{
		char buf[256];
		sprintf(buf,"ORDER_%s",OrderTreeData[i].KeyName);
		OrderPos[OrderTreeData[i].ID-1]=(BYTE)DBGetContactSettingByte(NULL,CLUIFrameModule,buf,i);
	}
	return 0;
}
int dragging=0;
HANDLE hDragItem=NULL;

BOOL CALLBACK DlgProcExtraIconsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_EXTRAORDER),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_EXTRAORDER),GWL_STYLE)|TVS_NOHSCROLL);

			{	
				HIMAGELIST himlCheckBoxes;
				himlCheckBoxes=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,2,2);
				ImageList_AddIcon(himlCheckBoxes,LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_NOTICK)));
				ImageList_AddIcon(himlCheckBoxes,LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_TICK)));
				TreeView_SetImageList(GetDlgItem(hwndDlg,IDC_EXTRAORDER),himlCheckBoxes,TVSIL_NORMAL);
			}
			FillOrderTree(hwndDlg,GetDlgItem(hwndDlg,IDC_EXTRAORDER));
			return TRUE;
		}

	case WM_COMMAND:
		{
			SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, (WPARAM)hwndDlg, 0);
			break;
		}
	case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->idFrom) 
			{		
			case 0: 
				{
					switch (((LPNMHDR)lParam)->code)
					{
					case PSN_APPLY:
						{
							SaveOrderTree(hwndDlg,GetDlgItem(hwndDlg,IDC_EXTRAORDER));      
							ReAssignExtraIcons();	
							ReloadCLUIOptions();
							return TRUE;
						}
					}
				}
			case IDC_EXTRAORDER:
				{
					switch (((LPNMHDR)lParam)->code) 
					{
					case TVN_BEGINDRAGA:
					case TVN_BEGINDRAGW:
						SetCapture(hwndDlg);
						dragging=1;
						hDragItem=((LPNMTREEVIEWA)lParam)->itemNew.hItem;
						TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),hDragItem);
						break;
					case NM_CLICK:
						{						
							TVHITTESTINFO hti;
							hti.pt.x=(short)LOWORD(GetMessagePos());
							hti.pt.y=(short)HIWORD(GetMessagePos());
							ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
							if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti))
								if(hti.flags&TVHT_ONITEMICON) 
								{
									TVITEMA tvi;
									tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
									tvi.hItem=hti.hItem;
									TreeView_GetItemA(((LPNMHDR)lParam)->hwndFrom,&tvi);
									tvi.iImage=tvi.iSelectedImage=!tvi.iImage;
									((TOrderTreeData *)tvi.lParam)->Visible=tvi.iImage;
									TreeView_SetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
									SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, (WPARAM)hwndDlg, 0);
								}

						};
					}
					break;
				}
			} 
			break;
		}
	case WM_MOUSEMOVE:
		{
			if(!dragging) break;
			{	
				TVHITTESTINFO hti;
				hti.pt.x=(short)LOWORD(lParam);
				hti.pt.y=(short)HIWORD(lParam);
				ClientToScreen(hwndDlg,&hti.pt);
				ScreenToClient(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&hti.pt);
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&hti);
				if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT))
				{
					HTREEITEM it=hti.hItem;
					hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_EXTRAORDER))/2;
					TreeView_HitTest(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&hti);
					//TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_EXTRAORDER),hti.hItem,1);
					if (!(hti.flags&TVHT_ABOVE))
					    TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_EXTRAORDER),hti.hItem,1);
                    else 
                        TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_EXTRAORDER),it,0);
				}
				else 
				{
					if(hti.flags&TVHT_ABOVE) SendDlgItemMessage(hwndDlg,IDC_EXTRAORDER,WM_VSCROLL,MAKEWPARAM(SB_LINEUP,0),0);
					if(hti.flags&TVHT_BELOW) SendDlgItemMessage(hwndDlg,IDC_EXTRAORDER,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),0);
					TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_EXTRAORDER),NULL,0);
				}
			}	
		}
		break;
	case WM_LBUTTONUP:
		{
			if(!dragging) break;
			TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_EXTRAORDER),NULL,0);
			dragging=0;
			ReleaseCapture();
			{	
				TVHITTESTINFO hti;
				TVITEM tvi;
				hti.pt.x=(short)LOWORD(lParam);
				hti.pt.y=(short)HIWORD(lParam);
				ClientToScreen(hwndDlg,&hti.pt);
				ScreenToClient(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&hti.pt);
				hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_EXTRAORDER))/2;
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&hti);
				if(hDragItem==hti.hItem) break;
				if (hti.flags&TVHT_ABOVE) hti.hItem=TVI_FIRST;
				tvi.mask=TVIF_HANDLE|TVIF_PARAM;
				tvi.hItem=hDragItem;//hti.hItem;
				//if (hti.hItem!=TVI_FIRST) 
				TreeView_GetItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&tvi);
				if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT)||(hti.hItem==TVI_FIRST)) 
				{
					TVINSERTSTRUCT tvis;
					TCHAR name[128];
					tvis.item.mask=TVIF_HANDLE|TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
					tvis.item.stateMask=0xFFFFFFFF;
					tvis.item.pszText=name;
					tvis.item.cchTextMax=sizeof(name);
					tvis.item.hItem=hDragItem;
					//
					tvis.item.iImage=tvis.item.iSelectedImage=((TOrderTreeData *)tvi.lParam)->Visible;				
					TreeView_GetItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&tvis.item);				
					TreeView_DeleteItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),hDragItem);
					tvis.hParent=NULL;
					tvis.hInsertAfter=hti.hItem;
					TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),TreeView_InsertItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&tvis));
					SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, (WPARAM)hwndDlg, 0);
				}
			}
		}
		break; 
	}
	return FALSE;
}
BOOL MODE=FALSE;
BOOL CALLBACK DlgProcCluiOpts2(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		hCLUIwnd=hwndDlg;  
		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg, IDC_CLIENTDRAG, DBGetContactSettingByte(NULL,"CLUI","ClientAreaDrag",SETTING_CLIENTDRAG_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_DRAGTOSCROLL, (DBGetContactSettingByte(NULL,"CLUI","DragToScroll",0)&&!DBGetContactSettingByte(NULL,"CLUI","ClientAreaDrag",SETTING_CLIENTDRAG_DEFAULT)) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_AUTOSIZE, DBGetContactSettingByte(NULL,"CLUI","AutoSize",0) ? BST_CHECKED : BST_UNCHECKED);			
		CheckDlgButton(hwndDlg, IDC_LOCKSIZING, DBGetContactSettingByte(NULL,"CLUI","LockSize",0) ? BST_CHECKED : BST_UNCHECKED);			   
		SendDlgItemMessage(hwndDlg,IDC_MAXSIZESPIN,UDM_SETRANGE,0,MAKELONG(100,0));
		SendDlgItemMessage(hwndDlg,IDC_MAXSIZESPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","MaxSizeHeight",75));
		CheckDlgButton(hwndDlg, IDC_AUTOSIZEUPWARD, DBGetContactSettingByte(NULL,"CLUI","AutoSizeUpward",0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SNAPTOEDGES, DBGetContactSettingByte(NULL,"CLUI","SnapToEdges",0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_AUTOHIDE, DBGetContactSettingByte(NULL,"CList","AutoHide",SETTING_AUTOHIDE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_SETRANGE,0,MAKELONG(900,1));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","HideTime",SETTING_HIDETIME_DEFAULT),0));
		EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIME),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC01),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		{
			int i, item;
			TCHAR *hidemode[]={TranslateT("Hide to tray"), TranslateT("Behind left edge"), TranslateT("Behind right edge")};
			for (i=0; i<sizeof(hidemode)/sizeof(char*); i++) {
				item=SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_ADDSTRING,0,(LPARAM)(hidemode[i]));
				SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_SETITEMDATA,item,(LPARAM)0);
				SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_SETCURSEL,DBGetContactSettingByte(NULL,"ModernData","HideBehind",0),0);
			}
		}
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN2,UDM_SETRANGE,0,MAKELONG(600,0));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN2,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"ModernData","ShowDelay",3),0));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN3,UDM_SETRANGE,0,MAKELONG(600,0));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN3,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"ModernData","HideDelay",3),0));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN4,UDM_SETRANGE,0,MAKELONG(50,1));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN4,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"ModernData","HideBehindBorderSize",1),0));
		{
			int mode=SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_GETCURSEL,0,0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWDELAY),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN2),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN3),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN4),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY2),mode!=0);
		}

		if(!IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE)) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC21),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC22),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZEHEIGHT),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZESPIN),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_AUTOSIZEUPWARD),FALSE);
		}
		return TRUE;
	case WM_COMMAND:
		if(LOWORD(wParam)==IDC_AUTOHIDE) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIME),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC01),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		}
		else if(LOWORD(wParam)==IDC_DRAGTOSCROLL && IsDlgButtonChecked(hwndDlg,IDC_CLIENTDRAG)) {
			CheckDlgButton(hwndDlg,IDC_CLIENTDRAG,FALSE);
		}
		else if(LOWORD(wParam)==IDC_CLIENTDRAG && IsDlgButtonChecked(hwndDlg,IDC_DRAGTOSCROLL)) {
			CheckDlgButton(hwndDlg,IDC_DRAGTOSCROLL,FALSE);
		}
		else if(LOWORD(wParam)==IDC_AUTOSIZE) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC21),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC22),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZEHEIGHT),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_AUTOSIZEUPWARD),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
		}
		else if (LOWORD(wParam)==IDC_HIDEMETHOD)
		{
			int mode=SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_GETCURSEL,0,0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWDELAY),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN2),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN3),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN4),mode!=0);     
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY2),mode!=0);
		}
		if ((LOWORD(wParam)==IDC_HIDETIME || LOWORD(wParam)==IDC_HIDEDELAY2 ||LOWORD(wParam)==IDC_HIDEDELAY ||LOWORD(wParam)==IDC_SHOWDELAY || LOWORD(wParam)==IDC_MAXSIZEHEIGHT) &&
			(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))
			return 0;
		// Enable apply button
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		break;	
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_APPLY:
			//
			//DBWriteContactSettingByte(NULL,"CLUI","LeftClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_GETPOS,0,0));
			//DBWriteContactSettingByte(NULL,"CLUI","RightClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_GETPOS,0,0));
			//DBWriteContactSettingByte(NULL,"CLUI","TopClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_TOPMARGINSPIN,UDM_GETPOS,0,0));
			//DBWriteContactSettingByte(NULL,"CLUI","BottomClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_BOTTOMMARGINSPIN,UDM_GETPOS,0,0));
			//if (MyUpdateLayeredWindow!=NULL && IsDlgButtonChecked(hwndDlg,IDC_LAYERENGINE)) 
			//	DBWriteContactSettingByte(NULL,"ModernData","EnableLayering",0);
			//else 
			//	DBDeleteContactSetting(NULL,"ModernData","EnableLayering");	
			DBWriteContactSettingByte(NULL,"ModernData","HideBehind",(BYTE)SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_GETCURSEL,0,0));  
			DBWriteContactSettingWord(NULL,"ModernData","ShowDelay",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN2,UDM_GETPOS,0,0));
			DBWriteContactSettingWord(NULL,"ModernData","HideDelay",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN3,UDM_GETPOS,0,0));
			DBWriteContactSettingWord(NULL,"ModernData","HideBehindBorderSize",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN4,UDM_GETPOS,0,0));

			//    CheckDlgButton(hwndDlg, IDC_CHECKKEYCOLOR, DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",1) ? BST_CHECKED : BST_UNCHECKED);
			//    EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR));
			//    SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255)));

			//DBWriteContactSettingByte(NULL,"ModernSettings","UseKeyColor",IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR)?1:0);
			//DBWriteContactSettingDword(NULL,"ModernSettings","KeyColor",SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_GETCOLOUR,0,0));
			//UseKeyColor=DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",1);
			//KeyColor=DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255));
			//DBWriteContactSettingByte(NULL,"CList","OnDesktop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP));
			//DBWriteContactSettingByte(NULL,"CList","OnTop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONTOP));
			//SetWindowPos(pcli->hwndContactList, IsDlgButtonChecked(hwndDlg,IDC_ONTOP)?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |SWP_NOACTIVATE);
			DBWriteContactSettingByte(NULL,"CLUI","DragToScroll",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_DRAGTOSCROLL));
			DBWriteContactSettingByte(NULL,"CList","BringToFront",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_BRINGTOFRONT));
			IsInChangingMode=TRUE;
			DBWriteContactSettingByte(NULL,"CLUI","ClientAreaDrag",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_CLIENTDRAG));
			DBWriteContactSettingByte(NULL,"CLUI","AutoSize",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			DBWriteContactSettingByte(NULL,"CLUI","LockSize",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_LOCKSIZING));
			DBWriteContactSettingByte(NULL,"CLUI","MaxSizeHeight",(BYTE)GetDlgItemInt(hwndDlg,IDC_MAXSIZEHEIGHT,NULL,FALSE));               
			DBWriteContactSettingByte(NULL,"CLUI","AutoSizeUpward",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZEUPWARD));
			DBWriteContactSettingByte(NULL,"CLUI","SnapToEdges",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SNAPTOEDGES));
			DBWriteContactSettingByte(NULL,"CList","AutoHide",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
			DBWriteContactSettingWord(NULL,"CList","HideTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_GETPOS,0,0));

			ChangeWindowMode(); 
			SendMessage(pcli->hwndContactTree,WM_SIZE,0,0);	//forces it to send a cln_listsizechanged
			ReloadCLUIOptions();

			cliShowHide(0,1);
			IsInChangingMode=FALSE;
			return TRUE;
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		hCLUIwnd=hwndDlg;  
		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg, IDC_ONTOP, DBGetContactSettingByte(NULL,"CList","OnTop",SETTING_ONTOP_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);		
		{	// ====== Activate/Deactivate Non-Layered items =======
			MODE=!LayeredFlag;
			EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),MODE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),MODE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_BORDER),MODE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_NOBORDERWND),MODE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWCAPTION),MODE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWMAINMENU),MODE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),MODE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_DROPSHADOW),MODE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_TITLEBAR_STATIC),MODE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),!MODE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_CHECKKEYCOLOR),!MODE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ROUNDCORNERS),MODE);
		}
		{   // ====== Non-Layered Mode =====
			CheckDlgButton(hwndDlg, IDC_TOOLWND, DBGetContactSettingByte(NULL,"CList","ToolWindow",SETTING_TOOLWINDOW_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_MIN2TRAY, DBGetContactSettingByte(NULL,"CList","Min2Tray",SETTING_MIN2TRAY_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_BORDER, DBGetContactSettingByte(NULL,"CList","ThinBorder",0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_NOBORDERWND, DBGetContactSettingByte(NULL,"CList","NoBorder",0) ? BST_CHECKED : BST_UNCHECKED);
			if(IsDlgButtonChecked(hwndDlg,IDC_TOOLWND))
				EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),FALSE);
			CheckDlgButton(hwndDlg, IDC_SHOWCAPTION, DBGetContactSettingByte(NULL,"CLUI","ShowCaption",SETTING_SHOWCAPTION_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWMAINMENU, DBGetContactSettingByte(NULL,"CLUI","ShowMainMenu",SETTING_SHOWMAINMENU_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			//EnableWindow(GetDlgItem(hwndDlg,IDC_CLIENTDRAG),!IsDlgButtonChecked(hwndDlg,IDC_DRAGTOSCROLL));
			if(!IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION)) 
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),FALSE);
			}
			if (IsDlgButtonChecked(hwndDlg,IDC_BORDER) || IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND))
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWCAPTION),FALSE);				
			}
			CheckDlgButton(hwndDlg, IDC_DROPSHADOW, DBGetContactSettingByte(NULL,"CList","WindowShadow",0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ROUNDCORNERS, DBGetContactSettingByte(NULL,"CLC","RoundCorners",0) ? BST_CHECKED : BST_UNCHECKED);	  
		}   // ====== End of Non-Layered Mode =====

		CheckDlgButton(hwndDlg, IDC_FADEINOUT, DBGetContactSettingByte(NULL,"CLUI","FadeInOut",1) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_ONDESKTOP, DBGetContactSettingByte(NULL,"CList","OnDesktop", 0) ? BST_CHECKED : BST_UNCHECKED);
		SendDlgItemMessage(hwndDlg,IDC_FRAMESSPIN,UDM_SETRANGE,0,MAKELONG(10,0));
		SendDlgItemMessage(hwndDlg,IDC_CAPTIONSSPIN,UDM_SETRANGE,0,MAKELONG(10,0));
		SendDlgItemMessage(hwndDlg,IDC_FRAMESSPIN,UDM_SETPOS,0,DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",1));
		SendDlgItemMessage(hwndDlg,IDC_CAPTIONSSPIN,UDM_SETPOS,0,DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",1));
		SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));
		SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));
		SendDlgItemMessage(hwndDlg,IDC_TOPMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));
		SendDlgItemMessage(hwndDlg,IDC_BOTTOMMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));
		SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","LeftClientMargin",0));
		SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","RightClientMargin",0));
		SendDlgItemMessage(hwndDlg,IDC_TOPMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","TopClientMargin",0));
		SendDlgItemMessage(hwndDlg,IDC_BOTTOMMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","BottomClientMargin",0));
		EnableWindow(GetDlgItem(hwndDlg,IDC_LAYERENGINE),(MyUpdateLayeredWindow!=NULL)?TRUE:FALSE);
		CheckDlgButton(hwndDlg, IDC_LAYERENGINE, (DBGetContactSettingByte(NULL,"ModernData","EnableLayering",1)&&MyUpdateLayeredWindow!=NULL) ? BST_UNCHECKED:BST_CHECKED);   
		CheckDlgButton(hwndDlg, IDC_CHECKKEYCOLOR, DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",1) ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),!MODE&&IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR));
		SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255)));	
		{
			DBVARIANT dbv={0};
		TCHAR *s=NULL;
		char szUin[20];
		if(!DBGetContactSettingTString(NULL,"CList","TitleText",&dbv))
			s=mir_tstrdup(dbv.ptszVal);
		else
			s=mir_tstrdup(_T(MIRANDANAME));
		//dbv.pszVal=s;
		SetDlgItemText(hwndDlg,IDC_TITLETEXT,s);
		if (s) mir_free(s);
		DBFreeVariant(&dbv);
		//if (s) mir_free(s);
		SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)MIRANDANAME);
		sprintf(szUin,"%u",DBGetContactSettingDword(NULL,"ICQ","UIN",0));
		SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)szUin);

		if(!DBGetContactSetting(NULL,"ICQ","Nick",&dbv)) {
			SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)dbv.pszVal);
			//mir_free(dbv.pszVal);
			DBFreeVariant(&dbv);
			dbv.pszVal=NULL;
		}
		if(!DBGetContactSetting(NULL,"ICQ","FirstName",&dbv)) {
			SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)dbv.pszVal);
			//mir_free(dbv.pszVal);
			DBFreeVariant(&dbv);
			dbv.pszVal=NULL;
		}
		if(!DBGetContactSetting(NULL,"ICQ","e-mail",&dbv)) {
			SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)dbv.pszVal);
			//mir_free(dbv.pszVal);
			DBFreeVariant(&dbv);
			dbv.pszVal=NULL;
		}
		}
		if(!IsWinVer2000Plus()) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_FADEINOUT),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENT),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_DROPSHADOW),FALSE);
		}
		else CheckDlgButton(hwndDlg,IDC_TRANSPARENT,DBGetContactSettingByte(NULL,"CList","Transparent",SETTING_TRANSPARENT_DEFAULT)?BST_CHECKED:BST_UNCHECKED);
		if(!IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT)) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC11),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC12),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSACTIVE),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSINACTIVE),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ACTIVEPERC),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_INACTIVEPERC),FALSE);
		}
		SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_SETRANGE,FALSE,MAKELONG(1,255));
		SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_SETRANGE,FALSE,MAKELONG(1,255));
		SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_SETPOS,TRUE,DBGetContactSettingByte(NULL,"CList","Alpha",SETTING_ALPHA_DEFAULT));
		SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_SETPOS,TRUE,DBGetContactSettingByte(NULL,"CList","AutoAlpha",SETTING_AUTOALPHA_DEFAULT));
		SendMessage(hwndDlg,WM_HSCROLL,0x12345678,0);
		return TRUE;

	case WM_COMMAND:
		if(LOWORD(wParam)==IDC_TRANSPARENT) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC11),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC12),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSACTIVE),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSINACTIVE),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg,IDC_ACTIVEPERC),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg,IDC_INACTIVEPERC),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
		}

		else if(LOWORD(wParam)==IDC_ONDESKTOP && IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP)) {
			CheckDlgButton(hwndDlg, IDC_ONTOP, BST_UNCHECKED);    
		}
		else if(LOWORD(wParam)==IDC_ONTOP && IsDlgButtonChecked(hwndDlg,IDC_ONTOP)) {
			CheckDlgButton(hwndDlg, IDC_ONDESKTOP, BST_UNCHECKED);    
		}
		else if(LOWORD(wParam)==IDC_TOOLWND) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),!IsDlgButtonChecked(hwndDlg,IDC_TOOLWND));
		}
		else if(LOWORD(wParam)==IDC_SHOWCAPTION) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
			EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),!IsDlgButtonChecked(hwndDlg,IDC_TOOLWND) && IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
		}
		else if(LOWORD(wParam)==IDC_NOBORDERWND ||LOWORD(wParam)==IDC_BORDER)
		{
			EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),(IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION))&&!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
			EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),(IsDlgButtonChecked(hwndDlg,IDC_TOOLWND) && IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION)) && !(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
			EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),(IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION))&&!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWCAPTION),!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));             
			if (LOWORD(wParam)==IDC_BORDER) CheckDlgButton(hwndDlg, IDC_NOBORDERWND,BST_UNCHECKED);
			else CheckDlgButton(hwndDlg, IDC_BORDER,BST_UNCHECKED); 

		}
		else if (LOWORD(wParam)==IDC_CHECKKEYCOLOR) 
		{
			EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR));
			SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255)));
		}
		if ((LOWORD(wParam)==IDC_TITLETEXT || LOWORD(wParam)==IDC_MAXSIZEHEIGHT || LOWORD(wParam)==IDC_FRAMESGAP || LOWORD(wParam)==IDC_CAPTIONSGAP ||
			  LOWORD(wParam)==IDC_LEFTMARGIN || LOWORD(wParam)==IDC_RIGHTMARGIN|| LOWORD(wParam)==IDC_TOPMARGIN || LOWORD(wParam)==IDC_BOTTOMMARGIN) 
			  &&(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))
			return 0;
		// Enable apply button
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		break;

	case WM_HSCROLL:
		{	char str[10];
		sprintf(str,"%d%%",100*SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_GETPOS,0,0)/255);
		SetDlgItemTextA(hwndDlg,IDC_INACTIVEPERC,str);
		sprintf(str,"%d%%",100*SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_GETPOS,0,0)/255);
		SetDlgItemTextA(hwndDlg,IDC_ACTIVEPERC,str);
		}
		if(wParam!=0x12345678) SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_APPLY:
			//
			DBWriteContactSettingByte(NULL,"CLUI","LeftClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CLUI","RightClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CLUI","TopClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_TOPMARGINSPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CLUI","BottomClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_BOTTOMMARGINSPIN,UDM_GETPOS,0,0));
			if (MyUpdateLayeredWindow!=NULL && IsDlgButtonChecked(hwndDlg,IDC_LAYERENGINE)) 
				DBWriteContactSettingByte(NULL,"ModernData","EnableLayering",0);
			else 
				DBDeleteContactSetting(NULL,"ModernData","EnableLayering");	
			//DBWriteContactSettingByte(NULL,"ModernData","HideBehind",(BYTE)SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_GETCURSEL,0,0));  
			//DBWriteContactSettingWord(NULL,"ModernData","ShowDelay",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN2,UDM_GETPOS,0,0));
			//DBWriteContactSettingWord(NULL,"ModernData","HideDelay",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN3,UDM_GETPOS,0,0));
			//DBWriteContactSettingWord(NULL,"ModernData","HideBehindBorderSize",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN4,UDM_GETPOS,0,0));

			//    CheckDlgButton(hwndDlg, IDC_CHECKKEYCOLOR, DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",1) ? BST_CHECKED : BST_UNCHECKED);
			//    EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR));
			//    SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255)));

			DBWriteContactSettingByte(NULL,"ModernSettings","UseKeyColor",IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR)?1:0);
			DBWriteContactSettingDword(NULL,"ModernSettings","KeyColor",SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_GETCOLOUR,0,0));
			UseKeyColor=DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",1);
			KeyColor=DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255));
			DBWriteContactSettingByte(NULL,"CList","OnDesktop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP));
			DBWriteContactSettingByte(NULL,"CList","OnTop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONTOP));
			SetWindowPos(pcli->hwndContactList, IsDlgButtonChecked(hwndDlg,IDC_ONTOP)?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |SWP_NOACTIVATE);
			DBWriteContactSettingByte(NULL,"CLUI","DragToScroll",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_DRAGTOSCROLL));
			//DBWriteContactSettingByte(NULL,"CList","BringToFront",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_BRINGTOFRONT));
			{ //====== Non-Layered Mode ======
				DBWriteContactSettingByte(NULL,"CList","ToolWindow",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_TOOLWND));
				DBWriteContactSettingByte(NULL,"CLUI","ShowCaption",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
				DBWriteContactSettingByte(NULL,"CLUI","ShowMainMenu",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWMAINMENU));
				DBWriteContactSettingByte(NULL,"CList","ThinBorder",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_BORDER));
				DBWriteContactSettingByte(NULL,"CList","NoBorder",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND));
				{	
					TCHAR title[256];
					GetDlgItemText(hwndDlg,IDC_TITLETEXT,title,SIZEOF(title));
					DBWriteContactSettingTString(NULL,"CList","TitleText",title);
					//			SetWindowText(pcli->hwndContactList,title);
				}
				DBWriteContactSettingByte(NULL,"CList","Min2Tray",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_MIN2TRAY));
				DBWriteContactSettingByte(NULL,"CList","WindowShadow",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_DROPSHADOW));
				DBWriteContactSettingByte(NULL,"CLC","RoundCorners",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ROUNDCORNERS));		
			} //====== End of Non-Layered Mode ======
			IsInChangingMode=TRUE;
			if (IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP)) 
			{
				HWND hProgMan=FindWindow(TEXT("Progman"),NULL);
				if (IsWindow(hProgMan)) 
				{
					SetParent(pcli->hwndContactList,hProgMan);
					SetParentForContainers(hProgMan);
					IsOnDesktop=1;
				}
			} 
			else 
			{
				if (GetParent(pcli->hwndContactList))
				{
					SetParent(pcli->hwndContactList,NULL);
					SetParentForContainers(NULL);
				}
				IsOnDesktop=0;
			}


			//DBWriteContactSettingByte(NULL,"CLUI","ClientAreaDrag",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_CLIENTDRAG));
			DBWriteContactSettingByte(NULL,"CLUI","FadeInOut",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_FADEINOUT));
			SmoothAnimation=IsWinVer2000Plus()&&(BYTE)IsDlgButtonChecked(hwndDlg,IDC_FADEINOUT);

			//DBWriteContactSettingByte(NULL,"CLUI","AutoSize",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			//DBWriteContactSettingByte(NULL,"CLUI","LockSize",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_LOCKSIZING));
			//DBWriteContactSettingByte(NULL,"CLUI","MaxSizeHeight",(BYTE)GetDlgItemInt(hwndDlg,IDC_MAXSIZEHEIGHT,NULL,FALSE));               
			{
				int i1;
				int i2;
				i1=SendDlgItemMessage(hwndDlg,IDC_FRAMESSPIN,UDM_GETPOS,0,0);
				i2=SendDlgItemMessage(hwndDlg,IDC_CAPTIONSSPIN,UDM_GETPOS,0,0);   

				DBWriteContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",(DWORD)i1);
				DBWriteContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",(DWORD)i2);
				CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
			}
			//DBWriteContactSettingByte(NULL,"CLUI","AutoSizeUpward",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZEUPWARD));
			//DBWriteContactSettingByte(NULL,"CLUI","SnapToEdges",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SNAPTOEDGES));
			//DBWriteContactSettingByte(NULL,"CList","AutoHide",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
			//DBWriteContactSettingWord(NULL,"CList","HideTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_GETPOS,0,0));

			DBWriteContactSettingByte(NULL,"CList","Transparent",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			DBWriteContactSettingByte(NULL,"CList","Alpha",(BYTE)SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CList","AutoAlpha",(BYTE)SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CList","OnDesktop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP));

			//if(IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT))	{
			//	SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) | WS_EX_LAYERED);
			//	if(MySetLayeredWindowAttributes) MySetLayeredWindowAttributes(pcli->hwndContactList, RGB(0,0,0), (BYTE)DBGetContactSettingByte(NULL,"CList","AutoAlpha",SETTING_AUTOALPHA_DEFAULT), LWA_ALPHA);
			//}
			//else {
			//	SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) & ~WS_EX_LAYERED);
			//}

			ChangeWindowMode(); 
			SendMessage(pcli->hwndContactTree,WM_SIZE,0,0);	//forces it to send a cln_listsizechanged
			ReloadCLUIOptions();

			cliShowHide(0,1);
			IsInChangingMode=FALSE;
			return TRUE;
		}
		break;
	}
	return FALSE;
}

LOGFONTA lf;



BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg, IDC_SHOWSBAR, DBGetContactSettingByte(NULL,"CLUI","ShowSBar",1) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_USECONNECTINGICON, DBGetContactSettingByte(NULL,"CLUI","UseConnectingIcon",1) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWXSTATUSNAME, ((DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",6)&8)>0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWXSTATUS, ((DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",6)&3)>0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWNORMAL, ((DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",6)&3)==2) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWBOTH, ((DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",6)&3)==3) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_TRANSPARENTOVERLAY, ((DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",6)&4)) ? BST_CHECKED : BST_UNCHECKED);
		{
			BYTE showOpts=DBGetContactSettingByte(NULL,"CLUI","SBarShow",7);
			CheckDlgButton(hwndDlg, IDC_SHOWICON, showOpts&1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWPROTO, showOpts&2 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWSTATUS, showOpts&4 ? BST_CHECKED : BST_UNCHECKED);
		}
		CheckDlgButton(hwndDlg, IDC_RIGHTSTATUS, DBGetContactSettingByte(NULL,"CLUI","SBarRightClk",0) ? BST_UNCHECKED : BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_RIGHTMIRANDA, !IsDlgButtonChecked(hwndDlg,IDC_RIGHTSTATUS) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_EQUALSECTIONS, DBGetContactSettingByte(NULL,"CLUI","EqualSections",0) ? BST_CHECKED : BST_UNCHECKED);

		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN,UDM_SETRANGE,0,MAKELONG(50,0));
		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingDword(NULL,"CLUI","LeftOffset",0),0));

		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN2,UDM_SETRANGE,0,MAKELONG(50,0));
		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN2,UDM_SETPOS,0,MAKELONG(DBGetContactSettingDword(NULL,"CLUI","RightOffset",0),0));

		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN3,UDM_SETRANGE,0,MAKELONG(50,0));
		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN3,UDM_SETPOS,0,MAKELONG(DBGetContactSettingDword(NULL,"CLUI","SpaceBetween",0),2));

		{
			int i, item;
			TCHAR *align[]={_T("Left"), _T("Center"), _T("Right")};
			for (i=0; i<sizeof(align)/sizeof(char*); i++) {
				item=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)TranslateTS(align[i]));
				SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETCURSEL,DBGetContactSettingByte(NULL,"CLUI","Align",0),0);
			}
		}
		{
			int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWPROTO),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWSTATUS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTSTATUS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTMIRANDA),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EQUALSECTIONS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_USECONNECTINGICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_USEOWNERDRAW),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN3),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON3),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON1),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_COMBO2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWXSTATUSNAME),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWXSTATUS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWBOTH),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && !IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
		}
		return TRUE;
	case WM_COMMAND:
		if(LOWORD(wParam)==IDC_BUTTON1)  
		{ 
			if (HIWORD(wParam)==BN_CLICKED)
			{
				CHOOSEFONTA fnt;
				memset(&fnt,0,sizeof(CHOOSEFONTA));
				fnt.lStructSize=sizeof(CHOOSEFONTA);
				fnt.hwndOwner=hwndDlg;
				fnt.Flags=CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT;
				fnt.lpLogFont=&lf;
				ChooseFontA(&fnt);
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
				return 0;
			} 
		}
		else if (LOWORD(wParam)==IDC_COLOUR ||(LOWORD(wParam)==IDC_COMBO2 && HIWORD(wParam)==CBN_SELCHANGE)) SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		else if (LOWORD(wParam)==IDC_SHOWSBAR) {
			int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWPROTO),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWSTATUS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTSTATUS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTMIRANDA),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EQUALSECTIONS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_USECONNECTINGICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_USEOWNERDRAW),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN3),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON3),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_COMBO2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON1),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWXSTATUSNAME),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWXSTATUS),en);

			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWBOTH),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && !IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));


			SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);	  
		}
		else if (LOWORD(wParam)==IDC_SHOWXSTATUS)	
		{
			int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWBOTH),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && !IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);	
		}
		else if (LOWORD(wParam)==IDC_SHOWBOTH)
		{
			int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
			if (IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH))	CheckDlgButton(hwndDlg,IDC_SHOWNORMAL,FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);	
		}
		else if (LOWORD(wParam)==IDC_SHOWNORMAL)	
		{
			int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
			if (IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL))	CheckDlgButton(hwndDlg,IDC_SHOWBOTH,FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWBOTH),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && !IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));       
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));

			SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);	
		}
		else if ((LOWORD(wParam)==IDC_OFFSETICON||LOWORD(wParam)==IDC_OFFSETICON2||LOWORD(wParam)==IDC_OFFSETICON3) && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap 
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_APPLY:
			{             
				//DBWriteContactSettingByte(NULL,"CLUI","ShowSBar",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR));
				DBWriteContactSettingByte(NULL,"CLUI","SBarShow",(BYTE)((IsDlgButtonChecked(hwndDlg,IDC_SHOWICON)?1:0)|(IsDlgButtonChecked(hwndDlg,IDC_SHOWPROTO)?2:0)|(IsDlgButtonChecked(hwndDlg,IDC_SHOWSTATUS)?4:0)));
				DBWriteContactSettingByte(NULL,"CLUI","SBarRightClk",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_RIGHTMIRANDA));
				DBWriteContactSettingByte(NULL,"CLUI","EqualSections",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_EQUALSECTIONS));
				DBWriteContactSettingByte(NULL,"CLUI","UseConnectingIcon",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_USECONNECTINGICON));
				DBWriteContactSettingDword(NULL,"CLUI","LeftOffset",(DWORD)SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN,UDM_GETPOS,0,0));
				DBWriteContactSettingDword(NULL,"CLUI","RightOffset",(DWORD)SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN2,UDM_GETPOS,0,0));
				DBWriteContactSettingDword(NULL,"CLUI","SpaceBetween",(DWORD)SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN3,UDM_GETPOS,0,0));
				DBWriteContactSettingByte(NULL,"CLUI","Align",(BYTE)SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETCURSEL,0,0));
				{
					BYTE val=0;
					if (IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS))
					{
						if (IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH)) val=3;
						else if (IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)) val=2;
						else val=1;
						val+=IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENTOVERLAY)?4:0;
					}
					val+=IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUSNAME)?8:0;
					DBWriteContactSettingByte(NULL,"CLUI","ShowXStatus",val);
				}	
				DBWriteContactSettingDword(NULL,"ModernData","StatusBarFontCol",SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0));
				DBWriteContactSettingByte(NULL,"CLUI","ShowSBar",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR));

				LoadStatusBarData();
				CluiProtocolStatusChanged(0,0);	
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}