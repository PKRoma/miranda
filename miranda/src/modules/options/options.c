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
#include "../../core/commonheaders.h"

static HANDLE hOptionsInitEvent;
static HWND hwndOptions=NULL;

struct OptionsPageInit {
	int pageCount;
	OPTIONSDIALOGPAGE *odp;
};

struct DlgTemplateExBegin {  
    WORD   dlgVer; 
    WORD   signature; 
    DWORD  helpID; 
    DWORD  exStyle; 
    DWORD  style; 
    WORD   cDlgItems; 
    short  x; 
    short  y; 
    short  cx; 
    short  cy; 
};

struct OptionsPageData {
	DLGTEMPLATE *pTemplate;
	DLGPROC dlgProc;
	HINSTANCE hInst;
	HTREEITEM hTreeItem;
	HWND hwnd;
	int changed;
	int simpleHeight,expertHeight;
	int simpleWidth,expertWidth;
	int simpleBottomControlId,simpleRightControlId;
	int nExpertOnlyControls;
	UINT *expertOnlyControls;
	DWORD flags;
	char *pszTitle,*pszGroup;
};

struct OptionsDlgData {
	int pageCount;
	int currentPage;
	HTREEITEM hCurrentPage;
	struct OptionsPageData *opd;
	RECT rcDisplay;
	HFONT hBoldFont;
};

static HTREEITEM FindNamedTreeItemAtRoot(HWND hwndTree,const char *name)
{
	TVITEM tvi;
	char str[128];

	tvi.mask=TVIF_TEXT;
	tvi.pszText=str;
	tvi.cchTextMax=sizeof(str);
	tvi.hItem=TreeView_GetRoot(hwndTree);
	while(tvi.hItem!=NULL) {
		TreeView_GetItem(hwndTree,&tvi);
		if(!strcmpi(str,name)) return tvi.hItem;
		tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
	}
	return NULL;
}

static BOOL CALLBACK BoldGroupTitlesEnumChildren(HWND hwnd,LPARAM lParam)
{
	char szClass[64];

	GetClassName(hwnd,szClass,sizeof(szClass));
	if(!lstrcmp(szClass,"Button") && (GetWindowLong(hwnd,GWL_STYLE)&0x0F)==BS_GROUPBOX)
		SendMessage(hwnd,WM_SETFONT,lParam,0);
	return TRUE;
}

#define OPTSTATE_PREFIX "s_"
static void SaveOptionsTreeState(HWND hdlg) {
	TVITEM tvi;
	char buf[130],str[128];
	tvi.mask=TVIF_TEXT|TVIF_STATE;
	tvi.pszText=str;
	tvi.cchTextMax=sizeof(str);
	tvi.hItem=TreeView_GetRoot(GetDlgItem(hdlg,IDC_PAGETREE));
	while(tvi.hItem!=NULL) {
		TreeView_GetItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvi);
		if (str) {
			wsprintf(buf,"%s%s",OPTSTATE_PREFIX,str);
			DBWriteContactSettingByte(NULL,"Options",buf,(BYTE)((tvi.state&TVIS_EXPANDED)?1:0));
		}
		tvi.hItem=TreeView_GetNextSibling(GetDlgItem(hdlg,IDC_PAGETREE),tvi.hItem);
	}
}

#define DM_FOCUSPAGE   (WM_USER+10)
#define DM_REBUILDPAGETREE (WM_USER+11)
static BOOL CALLBACK OptionsDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	struct OptionsDlgData *dat;

	dat=(struct OptionsDlgData*)GetWindowLong(hdlg,GWL_USERDATA);
	switch(message) {
		case WM_INITDIALOG:
		{	HRSRC hrsrc;
			HGLOBAL hglb;			
			PROPSHEETHEADER *psh=(PROPSHEETHEADER*)lParam;
			OPENOPTIONSDIALOG *ood=(OPENOPTIONSDIALOG*)psh->pStartPage;
			OPTIONSDIALOGPAGE *odp;
			int i;
			POINT pt;
			RECT rc,rcDlg;
			struct DlgTemplateExBegin *dte;
			DBVARIANT dbvLastGroup={DBVT_DELETED},dbvLastPage={DBVT_DELETED};

			Utils_RestoreWindowPositionNoSize(hdlg, NULL, "Options", "");
			TranslateDialogDefault(hdlg);
			SendMessage(hdlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_OPTIONS)));
			CheckDlgButton(hdlg,IDC_EXPERT,DBGetContactSettingByte(NULL,"Options","Expert",SETTING_SHOWEXPERT_DEFAULT)?BST_CHECKED:BST_UNCHECKED);
			EnableWindow(GetDlgItem(hdlg,IDC_APPLY),FALSE);
			dat=(struct OptionsDlgData*)malloc(sizeof(struct OptionsDlgData));
			SetWindowLong(hdlg,GWL_USERDATA,(LONG)dat);
			SetWindowText(hdlg,psh->pszCaption);
			{	LOGFONT lf;
				dat->hBoldFont=(HFONT)SendDlgItemMessage(hdlg,IDC_EXPERT,WM_GETFONT,0,0);
				GetObject(dat->hBoldFont,sizeof(lf),&lf);
				lf.lfWeight=FW_BOLD;
				dat->hBoldFont=CreateFontIndirect(&lf);
			}
			dat->pageCount=psh->nPages;
			dat->opd=(struct OptionsPageData*)malloc(sizeof(struct OptionsPageData)*dat->pageCount);
			odp=(OPTIONSDIALOGPAGE*)psh->ppsp;

			dat->currentPage=-1;
			if(ood->pszPage==NULL) {
				if(!DBGetContactSetting(NULL,"Options","LastGroup",&dbvLastGroup))
					ood->pszGroup=dbvLastGroup.pszVal;
				else ood->pszGroup=NULL;
				if(!DBGetContactSetting(NULL,"Options","LastPage",&dbvLastPage))
					ood->pszPage=dbvLastPage.pszVal;
				else ood->pszPage=NULL;
			}

			for(i=0;i<dat->pageCount;i++) {
				DWORD resSize;				
				hrsrc=FindResource(odp[i].hInstance,odp[i].pszTemplate,RT_DIALOG);
				hglb=LoadResource(odp[i].hInstance,hrsrc);
				resSize=SizeofResource(odp[i].hInstance,hrsrc);
				dat->opd[i].pTemplate=malloc(resSize);
				memcpy(dat->opd[i].pTemplate,LockResource(hglb),resSize);
				dte=(struct DlgTemplateExBegin*)dat->opd[i].pTemplate;
				if(dte->signature==0xFFFF) {
					//this feels like an access violation, and is according to boundschecker
					//...but it works - for now
					//may well have to remove and sort out the original dialogs
					dte->style&=~(WS_VISIBLE|WS_CHILD|WS_POPUP|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER);
					dte->style|=WS_CHILD;
				}
				else {
					dat->opd[i].pTemplate->style&=~(WS_VISIBLE|WS_CHILD|WS_POPUP|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER);
					dat->opd[i].pTemplate->style|=WS_CHILD;
				}
				dat->opd[i].dlgProc=odp[i].pfnDlgProc;
				dat->opd[i].hInst=odp[i].hInstance;
				dat->opd[i].hwnd=NULL;
				dat->opd[i].changed=0;
				dat->opd[i].simpleHeight=dat->opd[i].expertHeight=0;
				dat->opd[i].simpleBottomControlId=odp[i].nIDBottomSimpleControl;
				dat->opd[i].simpleWidth=dat->opd[i].expertWidth=0;
				dat->opd[i].simpleRightControlId=odp[i].nIDRightSimpleControl;
				dat->opd[i].nExpertOnlyControls=odp[i].nExpertOnlyControls;
				dat->opd[i].expertOnlyControls=odp[i].expertOnlyControls;
				dat->opd[i].flags=odp[i].flags;
				dat->opd[i].pszTitle=odp[i].pszTitle;
				if(odp[i].pszTitle) dat->opd[i].pszTitle=_strdup(odp[i].pszTitle);
				dat->opd[i].pszGroup=odp[i].pszGroup;
				if(odp[i].pszGroup) dat->opd[i].pszGroup=_strdup(odp[i].pszGroup);
				if(ood->pszPage && !lstrcmp(ood->pszPage,odp[i].pszTitle) &&
				   ((ood->pszGroup==NULL && odp[i].pszGroup==NULL) || (ood->pszGroup && odp[i].pszGroup && !lstrcmp(ood->pszGroup,odp[i].pszGroup))))
				    dat->currentPage=i;
			}
			DBFreeVariant(&dbvLastGroup);
			DBFreeVariant(&dbvLastPage);

			GetWindowRect(hdlg,&rcDlg);
			pt.x=pt.y=0;
			ClientToScreen(hdlg,&pt);
			GetWindowRect(GetDlgItem(hdlg,IDC_PAGETREE),&rc);
			dat->rcDisplay.left=rc.right-pt.x+(rc.left-rcDlg.left);
			dat->rcDisplay.top=rc.top-pt.y;
			dat->rcDisplay.right=rcDlg.right-(rc.left-rcDlg.left)-pt.x;
			GetWindowRect(GetDlgItem(hdlg,IDOK),&rc);
			dat->rcDisplay.bottom=rc.top-(rcDlg.bottom-rc.bottom)-pt.y;

			SendMessage(hdlg,DM_REBUILDPAGETREE,0,0);
			return TRUE;
		}
		case DM_REBUILDPAGETREE:
		{	int i;
			TVINSERTSTRUCT tvis;
			TVITEM tvi;
			char str[128],buf[130];

			TreeView_SelectItem(GetDlgItem(hdlg,IDC_PAGETREE),NULL);
			if(dat->currentPage != (-1)) ShowWindow(dat->opd[dat->currentPage].hwnd,SW_HIDE);
			ShowWindow(GetDlgItem(hdlg,IDC_PAGETREE),SW_HIDE);	 //deleteall is annoyingly visible
			TreeView_DeleteAllItems(GetDlgItem(hdlg,IDC_PAGETREE));
			dat->hCurrentPage=NULL;
			tvis.hParent=NULL;
			tvis.hInsertAfter=TVI_SORT;
			tvis.item.mask=TVIF_TEXT|TVIF_STATE|TVIF_PARAM;
			tvis.item.state=tvis.item.stateMask=TVIS_EXPANDED;
			for(i=0;i<dat->pageCount;i++) {
				if(dat->opd[i].flags&ODPF_SIMPLEONLY && IsDlgButtonChecked(hdlg,IDC_EXPERT)) continue;
				if(dat->opd[i].flags&ODPF_EXPERTONLY && !IsDlgButtonChecked(hdlg,IDC_EXPERT)) continue;
				tvis.hParent=NULL;
				if(dat->opd[i].pszGroup!=NULL) {
					tvis.hParent=FindNamedTreeItemAtRoot(GetDlgItem(hdlg,IDC_PAGETREE),dat->opd[i].pszGroup);
					if(tvis.hParent==NULL) {
						tvis.item.lParam=-1;
						tvis.item.pszText=dat->opd[i].pszGroup;
						tvis.hParent=TreeView_InsertItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvis);
					}
				}
				else {
					TVITEM tvi;
					tvi.hItem=FindNamedTreeItemAtRoot(GetDlgItem(hdlg,IDC_PAGETREE),dat->opd[i].pszTitle);
					if(tvi.hItem!=NULL) {
						if(i==dat->currentPage) dat->hCurrentPage=tvi.hItem;
						tvi.mask=TVIF_PARAM;
						TreeView_GetItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvi);
						if(tvi.lParam==-1) {
							tvi.lParam=i;
							TreeView_SetItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvi);
							continue;
						}
					}
				}
				tvis.item.pszText=(char*)dat->opd[i].pszTitle;
				tvis.item.lParam=i;
				dat->opd[i].hTreeItem=TreeView_InsertItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvis);
				if(i==dat->currentPage) dat->hCurrentPage=dat->opd[i].hTreeItem;
			}
			tvi.mask=TVIF_TEXT|TVIF_STATE;
			tvi.pszText=str;
			tvi.cchTextMax=sizeof(str);
			tvi.hItem=TreeView_GetRoot(GetDlgItem(hdlg,IDC_PAGETREE));
			while(tvi.hItem!=NULL) {
				TreeView_GetItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvi);
				if (str) {
					wsprintf(buf,"%s%s",OPTSTATE_PREFIX,str);
					if (!DBGetContactSettingByte(NULL,"Options",buf,1))
						TreeView_Expand(GetDlgItem(hdlg,IDC_PAGETREE),tvi.hItem,TVE_COLLAPSE);
				}
				tvi.hItem=TreeView_GetNextSibling(GetDlgItem(hdlg,IDC_PAGETREE),tvi.hItem);
			}
			if(dat->hCurrentPage==NULL) dat->hCurrentPage=TreeView_GetRoot(GetDlgItem(hdlg,IDC_PAGETREE));
			dat->currentPage=-1;
			TreeView_SelectItem(GetDlgItem(hdlg,IDC_PAGETREE),dat->hCurrentPage);
			ShowWindow(GetDlgItem(hdlg,IDC_PAGETREE),SW_SHOW);
			break;
		}
		case PSM_CHANGED:
			EnableWindow(GetDlgItem(hdlg,IDC_APPLY),TRUE);
			if(dat->currentPage != (-1)) dat->opd[dat->currentPage].changed=1;
			return TRUE;
		case PSM_ISEXPERT:
			SetWindowLong(hdlg,DWL_MSGRESULT,IsDlgButtonChecked(hdlg,IDC_EXPERT));
			return TRUE;
		case PSM_GETBOLDFONT:
			SetWindowLong(hdlg,DWL_MSGRESULT,(LONG)dat->hBoldFont);
			return TRUE;
		case WM_NOTIFY:
			switch(wParam) {
				case IDC_PAGETREE:
					switch(((LPNMHDR)lParam)->code) {
						case TVN_ITEMEXPANDING:
							SetWindowLong(hdlg,DWL_MSGRESULT,FALSE);
							return TRUE;
   						case TVN_SELCHANGING:
						{	PSHNOTIFY pshn;
							if(dat->currentPage==-1 || dat->opd[dat->currentPage].hwnd==NULL) break;
							pshn.hdr.code=PSN_KILLACTIVE;
							pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
							pshn.hdr.idFrom=0;
							pshn.lParam=0;
							if(SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn)) {
								SetWindowLong(hdlg,DWL_MSGRESULT,TRUE);
								return TRUE;
							}
							break;
						}
						case TVN_SELCHANGED:
						{	TVITEM tvi;
							ShowWindow(GetDlgItem(hdlg,IDC_STNOPAGE),SW_HIDE);
							if(dat->currentPage!=-1 && dat->opd[dat->currentPage].hwnd!=NULL) ShowWindow(dat->opd[dat->currentPage].hwnd,SW_HIDE);
							tvi.hItem=dat->hCurrentPage=TreeView_GetSelection(GetDlgItem(hdlg,IDC_PAGETREE));
							if(tvi.hItem==NULL) break;
							tvi.mask=TVIF_HANDLE|TVIF_PARAM;
							TreeView_GetItem(GetDlgItem(hdlg,IDC_PAGETREE),&tvi);
							dat->currentPage=tvi.lParam;
							if(dat->currentPage!=-1) {
								if(dat->opd[dat->currentPage].hwnd==NULL) {
									RECT rcPage;
									RECT rcControl,rc;
									int w,h;

									dat->opd[dat->currentPage].hwnd=CreateDialogIndirect(dat->opd[dat->currentPage].hInst,dat->opd[dat->currentPage].pTemplate,hdlg,dat->opd[dat->currentPage].dlgProc);
									if(dat->opd[dat->currentPage].flags&ODPF_BOLDGROUPS)
										EnumChildWindows(dat->opd[dat->currentPage].hwnd,BoldGroupTitlesEnumChildren,(LPARAM)dat->hBoldFont);
									GetClientRect(dat->opd[dat->currentPage].hwnd,&rcPage);
									dat->opd[dat->currentPage].expertWidth=rcPage.right;
									dat->opd[dat->currentPage].expertHeight=rcPage.bottom;
									GetWindowRect(dat->opd[dat->currentPage].hwnd,&rc);
									if(dat->opd[dat->currentPage].simpleBottomControlId) {
										GetWindowRect(GetDlgItem(dat->opd[dat->currentPage].hwnd,dat->opd[dat->currentPage].simpleBottomControlId),&rcControl);
										dat->opd[dat->currentPage].simpleHeight=rcControl.bottom-rc.top;
									}
									else dat->opd[dat->currentPage].simpleHeight=dat->opd[dat->currentPage].expertHeight;
									if(dat->opd[dat->currentPage].simpleRightControlId) {
										GetWindowRect(GetDlgItem(dat->opd[dat->currentPage].hwnd,dat->opd[dat->currentPage].simpleRightControlId),&rcControl);
										dat->opd[dat->currentPage].simpleWidth=rcControl.right-rc.left;
									}
									else dat->opd[dat->currentPage].simpleWidth=dat->opd[dat->currentPage].expertWidth;
									if(IsDlgButtonChecked(hdlg,IDC_EXPERT)) {
										w=dat->opd[dat->currentPage].expertWidth;
										h=dat->opd[dat->currentPage].expertHeight;
									}
									else {
										int i;
										for(i=0;i<dat->opd[dat->currentPage].nExpertOnlyControls;i++)
											ShowWindow(GetDlgItem(dat->opd[dat->currentPage].hwnd,dat->opd[dat->currentPage].expertOnlyControls[i]),SW_HIDE);
										w=dat->opd[dat->currentPage].simpleWidth;
										h=dat->opd[dat->currentPage].simpleHeight;
									}
									SetWindowPos(dat->opd[dat->currentPage].hwnd,HWND_TOP,(dat->rcDisplay.left+dat->rcDisplay.right-w)>>1,(dat->rcDisplay.top+dat->rcDisplay.bottom-h)>>1,w,h,0);
								}
								ShowWindow(dat->opd[dat->currentPage].hwnd,SW_SHOW);
								if(((LPNMTREEVIEW)lParam)->action==TVC_BYMOUSE) PostMessage(hdlg,DM_FOCUSPAGE,0,0);
								else SetFocus(GetDlgItem(hdlg,IDC_PAGETREE));
							}
							else ShowWindow(GetDlgItem(hdlg,IDC_STNOPAGE),SW_SHOW);
							break;
						}
					}
					break;
			}
			break;
		case DM_FOCUSPAGE:
			if(dat->currentPage==-1) break;
			SetFocus(dat->opd[dat->currentPage].hwnd);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_EXPERT:
				{	int expert=IsDlgButtonChecked(hdlg,IDC_EXPERT);
					int i,j;
					PSHNOTIFY pshn;
					RECT rcPage;
					int neww,newh;

					DBWriteContactSettingByte(NULL,"Options","Expert",(BYTE)expert);
					pshn.hdr.idFrom=0;
					pshn.lParam=expert;
					pshn.hdr.code=PSN_EXPERTCHANGED;
					for(i=0;i<dat->pageCount;i++) {
						if(dat->opd[i].hwnd==NULL) continue;
						pshn.hdr.hwndFrom=dat->opd[i].hwnd;
						SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);

						for(j=0;j<dat->opd[i].nExpertOnlyControls;j++)
							ShowWindow(GetDlgItem(dat->opd[i].hwnd,dat->opd[i].expertOnlyControls[j]),expert?SW_SHOW:SW_HIDE);

						GetClientRect(dat->opd[i].hwnd,&rcPage);
						if(dat->opd[i].simpleBottomControlId) newh=expert?dat->opd[i].expertHeight:dat->opd[i].simpleHeight;
						else newh=rcPage.bottom-rcPage.top;
						if(dat->opd[i].simpleRightControlId) neww=expert?dat->opd[i].expertWidth:dat->opd[i].simpleWidth;
						else neww=rcPage.right-rcPage.left;
						if(i==dat->currentPage) {
							POINT ptStart,ptEnd,ptNow;
							DWORD thisTick,startTick;
							RECT rc;

							ptNow.x=ptNow.y=0;
							ClientToScreen(hdlg,&ptNow);
							GetWindowRect(dat->opd[i].hwnd,&rc);
							ptStart.x=rc.left-ptNow.x;
							ptStart.y=rc.top-ptNow.y;
							ptEnd.x=(dat->rcDisplay.left+dat->rcDisplay.right-neww)>>1;
							ptEnd.y=(dat->rcDisplay.top+dat->rcDisplay.bottom-newh)>>1;
							if(abs(ptEnd.x-ptStart.x)>5 || abs(ptEnd.y-ptStart.y)>5) {
								startTick=GetTickCount();
								SetWindowPos(dat->opd[i].hwnd,HWND_TOP,0,0,min(neww,rcPage.right),min(newh,rcPage.bottom),SWP_NOMOVE);
								UpdateWindow(dat->opd[i].hwnd);
								for(;;) {
									thisTick=GetTickCount();
									if(thisTick>startTick+100) break;
									ptNow.x=ptStart.x+(ptEnd.x-ptStart.x)*(int)(thisTick-startTick)/100;
									ptNow.y=ptStart.y+(ptEnd.y-ptStart.y)*(int)(thisTick-startTick)/100;
									SetWindowPos(dat->opd[i].hwnd,0,ptNow.x,ptNow.y,0,0,SWP_NOZORDER|SWP_NOSIZE);
								}
							}
						}
						SetWindowPos(dat->opd[i].hwnd,HWND_TOP,(dat->rcDisplay.left+dat->rcDisplay.right-neww)>>1,(dat->rcDisplay.top+dat->rcDisplay.bottom-newh)>>1,neww,newh,0);
					}
					SaveOptionsTreeState(hdlg);
					SendMessage(hdlg,DM_REBUILDPAGETREE,0,0);
					break;
				}
				case IDCANCEL:
				{	int i;
					PSHNOTIFY pshn;
					pshn.hdr.idFrom=0;
					pshn.lParam=0;
					pshn.hdr.code=PSN_RESET;
					for(i=0;i<dat->pageCount;i++) {
						if(dat->opd[i].hwnd==NULL || !dat->opd[i].changed) continue;
						pshn.hdr.hwndFrom=dat->opd[i].hwnd;
						SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);
					}
					DestroyWindow(hdlg);
					break;
				}
				case IDOK:
				case IDC_APPLY:
				{	int i;
					PSHNOTIFY pshn;
					EnableWindow(GetDlgItem(hdlg,IDC_APPLY),FALSE);
					if(dat->currentPage!=(-1)) {
						pshn.hdr.idFrom=0;
						pshn.lParam=0;
						pshn.hdr.code=PSN_KILLACTIVE;
						pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
						if(SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn))
							break;
					}

					pshn.hdr.code=PSN_APPLY;
					for(i=0;i<dat->pageCount;i++) {
						if(dat->opd[i].hwnd==NULL || !dat->opd[i].changed) continue;
						dat->opd[i].changed=0;
						pshn.hdr.hwndFrom=dat->opd[i].hwnd;
						if(SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn)==PSNRET_INVALID_NOCHANGEPAGE) {
							dat->hCurrentPage=dat->opd[i].hTreeItem;
							TreeView_SelectItem(GetDlgItem(hdlg,IDC_PAGETREE),dat->hCurrentPage);
							if(dat->currentPage!=(-1)) ShowWindow(dat->opd[dat->currentPage].hwnd,SW_HIDE);
							dat->currentPage=i;
							if (dat->currentPage != (-1)) ShowWindow(dat->opd[dat->currentPage].hwnd,SW_SHOW);
							return 0;
						}
					}
					if(LOWORD(wParam)==IDOK) DestroyWindow(hdlg);
					break;
				}
			}
			break;
		case WM_DESTROY:
		{	int i;
			SaveOptionsTreeState(hdlg);
			if(dat->currentPage!=-1) {
				if(dat->opd[dat->currentPage].pszGroup) DBWriteContactSettingString(NULL,"Options","LastGroup",dat->opd[dat->currentPage].pszGroup);
				else DBDeleteContactSetting(NULL,"Options","LastGroup");
				DBWriteContactSettingString(NULL,"Options","LastPage",dat->opd[dat->currentPage].pszTitle);
			}
			else {
				DBDeleteContactSetting(NULL,"Options","LastGroup");
				DBDeleteContactSetting(NULL,"Options","LastPage");
			}
			Utils_SaveWindowPosition(hdlg, NULL, "Options", "");
			for(i=0;i<dat->pageCount;i++) {
				if(dat->opd[i].hwnd!=NULL) DestroyWindow(dat->opd[i].hwnd);
				if(dat->opd[i].pszGroup) free(dat->opd[i].pszGroup);
				if(dat->opd[i].pszTitle) free(dat->opd[i].pszTitle);
				if(dat->opd[i].pTemplate) free(dat->opd[i].pTemplate);
			}
			free(dat->opd);
			DeleteObject(dat->hBoldFont);
			free(dat);
			hwndOptions=NULL;
			break;
		}
	}
	return FALSE;
}

static void OpenOptionsNow(const char *pszGroup,const char *pszPage)
{
	PROPSHEETHEADER psh;
	struct OptionsPageInit opi;
	int i;
	OPENOPTIONSDIALOG ood;

	if(IsWindow(hwndOptions)) {
		SetForegroundWindow(hwndOptions);
		return;
	}
	opi.pageCount=0;
	opi.odp=NULL;
	NotifyEventHooks(hOptionsInitEvent,(WPARAM)&opi,0);
	if(opi.pageCount==0) return;

	ZeroMemory(&psh,sizeof(psh));
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_PROPSHEETPAGE|PSH_NOAPPLYNOW;
	psh.hwndParent = NULL;
	psh.nPages = opi.pageCount;
	ood.pszGroup=pszGroup;
	ood.pszPage=pszPage;
	psh.pStartPage = (LPCTSTR)&ood;	  //more structure misuse
	psh.pszCaption = Translate("Miranda IM Options");	
	psh.ppsp = (PROPSHEETPAGE*)opi.odp;		  //blatent misuse of the structure, but what the hell
	hwndOptions=CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_OPTIONS),NULL,OptionsDlgProc,(LPARAM)&psh);
	for(i=0;i<opi.pageCount;i++) {
		free((char*)opi.odp[i].pszTitle);
		if(opi.odp[i].pszGroup!=NULL) free(opi.odp[i].pszGroup);
		if((DWORD)opi.odp[i].pszTemplate&0xFFFF0000) free((char*)opi.odp[i].pszTemplate);
	}
	free(opi.odp);
}

static int OpenOptions(WPARAM wParam,LPARAM lParam)
{
	OPENOPTIONSDIALOG *ood=(OPENOPTIONSDIALOG*)lParam;
	if(ood->cbSize!=sizeof(OPENOPTIONSDIALOG)) return 1;
	OpenOptionsNow(ood->pszGroup,ood->pszPage);
	return 0;
}

static int OpenOptionsDialog(WPARAM wParam,LPARAM lParam)
{
	OpenOptionsNow(NULL,NULL);
	return 0;
}

static int AddOptionsPage(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE *odp=(OPTIONSDIALOGPAGE*)lParam;
	struct OptionsPageInit *opi=(struct OptionsPageInit*)wParam;

	if(odp->cbSize!=sizeof(OPTIONSDIALOGPAGE) && odp->cbSize!=OPTIONSDIALOGPAGE_V0120_SIZE && odp->cbSize!=OPTIONSDIALOGPAGE_V0100_SIZE) return 1;
	opi->odp=(OPTIONSDIALOGPAGE*)realloc(opi->odp,sizeof(OPTIONSDIALOGPAGE)*(opi->pageCount+1));
	opi->odp[opi->pageCount].cbSize=sizeof(OPTIONSDIALOGPAGE);
	opi->odp[opi->pageCount].hInstance=odp->hInstance;
	opi->odp[opi->pageCount].pfnDlgProc=odp->pfnDlgProc;
	opi->odp[opi->pageCount].position=odp->position;
	opi->odp[opi->pageCount].pszTitle=_strdup(odp->pszTitle);
	if((DWORD)odp->pszTemplate&0xFFFF0000) opi->odp[opi->pageCount].pszTemplate=_strdup(odp->pszTemplate);
	else opi->odp[opi->pageCount].pszTemplate=odp->pszTemplate;
	if(odp->cbSize>OPTIONSDIALOGPAGE_V0120_SIZE) {
		opi->odp[opi->pageCount].flags=odp->flags;
		opi->odp[opi->pageCount].nIDBottomSimpleControl=odp->nIDBottomSimpleControl;
		opi->odp[opi->pageCount].nIDRightSimpleControl=odp->nIDRightSimpleControl;
		opi->odp[opi->pageCount].expertOnlyControls=odp->expertOnlyControls;
		opi->odp[opi->pageCount].nExpertOnlyControls=odp->nExpertOnlyControls;
	}
	else {
		opi->odp[opi->pageCount].flags=0;
		opi->odp[opi->pageCount].nIDBottomSimpleControl=0;
		opi->odp[opi->pageCount].nIDRightSimpleControl=0;
		opi->odp[opi->pageCount].expertOnlyControls=NULL;
		opi->odp[opi->pageCount].nExpertOnlyControls=0;
	}
	if(odp->cbSize>OPTIONSDIALOGPAGE_V0100_SIZE) {
		if(odp->pszGroup!=NULL) opi->odp[opi->pageCount].pszGroup=_strdup(Translate(odp->pszGroup));
		else opi->odp[opi->pageCount].pszGroup=NULL;
		opi->odp[opi->pageCount].groupPosition=odp->groupPosition;
		opi->odp[opi->pageCount].hGroupIcon=odp->hGroupIcon;
		opi->odp[opi->pageCount].hIcon=odp->hIcon;
	}
	else {
		opi->odp[opi->pageCount].pszGroup=NULL;
		opi->odp[opi->pageCount].groupPosition=0;
		opi->odp[opi->pageCount].hGroupIcon=NULL;
		opi->odp[opi->pageCount].hIcon=NULL;
	}
	opi->pageCount++;
	return 0;
}

static int OptModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM mi;

	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.flags=0;
	mi.hIcon=LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_OPTIONS));
	mi.position=1900000000;
	mi.pszName=Translate("&Options...");
	mi.pszService="Options/OptionsCommand";
	CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);
	return 0;
}

int ShutdownOptionsModule(WPARAM wParam,LPARAM lParam)
{
	if (IsWindow(hwndOptions)) DestroyWindow(hwndOptions);
	hwndOptions=NULL;
	return 0;
}

int LoadOptionsModule(void)
{
	hwndOptions=NULL;
	hOptionsInitEvent=CreateHookableEvent(ME_OPT_INITIALISE);
	CreateServiceFunction(MS_OPT_ADDPAGE,AddOptionsPage);
	CreateServiceFunction(MS_OPT_OPENOPTIONS,OpenOptions);
	CreateServiceFunction("Options/OptionsCommand",OpenOptionsDialog);
	HookEvent(ME_SYSTEM_MODULESLOADED,OptModulesLoaded);
	HookEvent(ME_SYSTEM_PRESHUTDOWN,ShutdownOptionsModule);
	return 0;
}
