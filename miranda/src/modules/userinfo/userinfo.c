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
#define UPDATEANIMFRAMES 20

int DetailsInit(WPARAM wParam,LPARAM lParam);
static BOOL CALLBACK DlgProcDetails(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static HANDLE hWindowList=NULL;
static HANDLE hDetailsInitEvent;

struct DetailsPageInit {
	int pageCount;
	OPTIONSDIALOGPAGE *odp;
};

struct DetailsPageData {
	DLGTEMPLATE *pTemplate;
	HINSTANCE hInst;
	DLGPROC dlgProc;
	HWND hwnd;
	int changed;
};

struct DetailsData {
	HANDLE hContact;
	HANDLE hProtoAckEvent;
	HINSTANCE hInstIcmp;
	HFONT hBoldFont;
	int pageCount;
	int currentPage;
	struct DetailsPageData *opd;
	RECT rcDisplay;
	int updateAnimFrame;
	char szUpdating[64];
	int *infosUpdated;
};

static int PageSortProc(OPTIONSDIALOGPAGE *item1,OPTIONSDIALOGPAGE *item2)
{
	if(item2->position>item1->position) return -1;
	if(item2->position<item1->position) return 1;
	return 0;
}

static int ShowDetailsDialogCommand(WPARAM wParam,LPARAM lParam)
{
	HWND hwnd;
	PROPSHEETHEADER psh;
	struct DetailsPageInit opi;
	int i;

	if(hwnd=WindowList_Find(hWindowList,(HANDLE)wParam)) {
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
		return 0;
	}

	opi.pageCount=0;
	opi.odp=NULL;
	NotifyEventHooks(hDetailsInitEvent,(WPARAM)&opi,wParam);
	if(opi.pageCount==0) return 0;
	qsort(opi.odp,opi.pageCount,sizeof(OPTIONSDIALOGPAGE),(int (*)(const void*,const void*))PageSortProc);

	ZeroMemory(&psh,sizeof(psh));
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_PROPSHEETPAGE|PSH_NOAPPLYNOW;
	psh.hwndParent = NULL;
	psh.nPages = opi.pageCount;
	psh.pStartPage = 0;
	psh.pszCaption = (char*)wParam;	  //more abuses of structure: this is hContact
	psh.ppsp = (PROPSHEETPAGE*)opi.odp;		  //blatent misuse of the structure, but what the hell
	CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DETAILS),NULL,DlgProcDetails,(LPARAM)&psh);
	for(i=0;i<opi.pageCount;i++) {
		free((char*)opi.odp[i].pszTitle);
		if(opi.odp[i].pszGroup!=NULL) free(opi.odp[i].pszGroup);
		if((DWORD)opi.odp[i].pszTemplate&0xFFFF0000) free((char*)opi.odp[i].pszTemplate);
	}
	free(opi.odp);
	return 0;
}

static int AddDetailsPage(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE *odp=(OPTIONSDIALOGPAGE*)lParam;
	struct DetailsPageInit *opi=(struct DetailsPageInit*)wParam;

	if(odp->cbSize!=sizeof(OPTIONSDIALOGPAGE) && odp->cbSize!=OPTIONSDIALOGPAGE_V0120_SIZE) return 1;
	opi->odp=(OPTIONSDIALOGPAGE*)realloc(opi->odp,sizeof(OPTIONSDIALOGPAGE)*(opi->pageCount+1));
	opi->odp[opi->pageCount].cbSize=sizeof(OPTIONSDIALOGPAGE);
	opi->odp[opi->pageCount].hInstance=odp->hInstance;
	opi->odp[opi->pageCount].pfnDlgProc=odp->pfnDlgProc;
	opi->odp[opi->pageCount].position=odp->position;
	opi->odp[opi->pageCount].pszTitle=_strdup(odp->pszTitle);
	if((DWORD)odp->pszTemplate&0xFFFF0000) opi->odp[opi->pageCount].pszTemplate=_strdup(odp->pszTemplate);
	else opi->odp[opi->pageCount].pszTemplate=odp->pszTemplate;
	opi->odp[opi->pageCount].pszGroup=NULL;
	opi->odp[opi->pageCount].groupPosition=odp->groupPosition;
	opi->odp[opi->pageCount].hGroupIcon=odp->hGroupIcon;
	opi->odp[opi->pageCount].hIcon=odp->hIcon;
	opi->pageCount++;
	return 0;
}

static int UserInfoContactDelete(WPARAM wParam,LPARAM lParam)
{
	HWND hwnd;
	hwnd=WindowList_Find(hWindowList,(HANDLE)wParam);
	if(hwnd!=NULL) DestroyWindow(hwnd);
	return 0;
}

static void ThemeDialogBackground(HWND hwnd) {
	if (IsWinVerXPPlus()) {
		static HMODULE hThemeAPI = NULL;
		if (!hThemeAPI) hThemeAPI = GetModuleHandle("uxtheme");
		if (hThemeAPI) {
			HRESULT (STDAPICALLTYPE *MyEnableThemeDialogTexture)(HWND,DWORD) = (HRESULT (STDAPICALLTYPE*)(HWND,DWORD))GetProcAddress(hThemeAPI,"EnableThemeDialogTexture");
			if (MyEnableThemeDialogTexture)
				MyEnableThemeDialogTexture(hwnd,0x00000002|0x00000004); //0x00000002|0x00000004=ETDT_ENABLETAB
		}
	}
}

#define HM_PROTOACK   (WM_USER+10)
#define M_CHECKONLINE (WM_USER+11)
static BOOL CALLBACK DlgProcDetails(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct DetailsData *dat;

	dat=(struct DetailsData*)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch (msg)
	{
		case WM_INITDIALOG:
		{	PROPSHEETHEADER *psh=(PROPSHEETHEADER*)lParam;
			TranslateDialogDefault(hwndDlg);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_USERDETAILS)));
			dat=(struct DetailsData*)malloc(sizeof(struct DetailsData));
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);
			dat->hContact=(HANDLE)psh->pszCaption;
			dat->hProtoAckEvent=HookEventMessage(ME_PROTO_ACK,hwndDlg,HM_PROTOACK);
			dat->infosUpdated=NULL;
			WindowList_Add(hWindowList,hwndDlg,dat->hContact);
			{
				char *name,oldTitle[256],newTitle[256];
				CallContactService(dat->hContact,PSS_GETINFO,0,0);

				if (dat->hContact == NULL)
					name = Translate("Owner");
				else
					name = (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)dat->hContact, 0);

				GetWindowText(hwndDlg,oldTitle,sizeof(oldTitle));
				mir_snprintf(newTitle,sizeof(newTitle),oldTitle,name);
				SetWindowText(hwndDlg,newTitle);
				SetDlgItemText(hwndDlg,IDC_NAME,name);

			}
			{	LOGFONT lf;
				HFONT hNormalFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_NAME,WM_GETFONT,0,0);
				GetObject(hNormalFont,sizeof(lf),&lf);
				lf.lfWeight=FW_BOLD;
				dat->hBoldFont=CreateFontIndirect(&lf);
				SendDlgItemMessage(hwndDlg,IDC_NAME,WM_SETFONT,(WPARAM)dat->hBoldFont,0);
			}
			{	OPTIONSDIALOGPAGE *odp;
				int i;
				TCITEM tci;
				DBVARIANT dbv;

				dat->currentPage=0;
				if(DBGetContactSetting(NULL,"UserInfo","LastTab",&dbv))
					dbv.type=DBVT_DELETED;
				dat->pageCount=psh->nPages;
				dat->opd=(struct DetailsPageData*)malloc(sizeof(struct DetailsPageData)*dat->pageCount);
				odp=(OPTIONSDIALOGPAGE*)psh->ppsp;

				tci.mask=TCIF_TEXT;
				for(i=0;i<dat->pageCount;i++) {
					dat->opd[i].pTemplate=(DLGTEMPLATE *)LockResource(LoadResource(odp[i].hInstance,FindResource(odp[i].hInstance,odp[i].pszTemplate,RT_DIALOG)));
					dat->opd[i].dlgProc=odp[i].pfnDlgProc;
					dat->opd[i].hInst=odp[i].hInstance;
					dat->opd[i].hwnd=NULL;
					dat->opd[i].changed=0;
					tci.pszText=(char*)odp[i].pszTitle;
					if(dbv.type!=DBVT_DELETED && !strcmp(tci.pszText,dbv.pszVal))
						dat->currentPage=i;
					TabCtrl_InsertItem(GetDlgItem(hwndDlg,IDC_TABS),i,&tci);
				}
				DBFreeVariant(&dbv);
			}
			
			
			GetWindowRect(GetDlgItem(hwndDlg,IDC_TABS),&dat->rcDisplay);
			TabCtrl_AdjustRect(GetDlgItem(hwndDlg,IDC_TABS),FALSE,&dat->rcDisplay);

			{	POINT pt={0,0};
				ClientToScreen(hwndDlg,&pt);
				OffsetRect(&dat->rcDisplay,-pt.x,-pt.y);
			}
			
			{ // Resize dialog to make room for all plugin tabs
				int displayheight=dat->rcDisplay.bottom - dat->rcDisplay.top;
				int needh;
				
				POINT mpt={0,0};
				RECT ws={0,0,0,0};
				ClientToScreen(hwndDlg,&mpt);
				
				// Map dlu to pixels
				ws.bottom=132;
				MapDialogRect(hwndDlg,&ws);
				needh=ws.bottom;
				
				if (displayheight<needh) {
					int addheight=needh-displayheight;
						
					// Resize whole dialog
					GetWindowRect(hwndDlg,&ws);
					MoveWindow(hwndDlg,
						ws.left,ws.top,
						ws.right-ws.left,ws.bottom-ws.top+addheight,
						FALSE);
					
					// Resize tabcontrol
					GetWindowRect(GetDlgItem(hwndDlg,IDC_TABS),&ws);
					OffsetRect(&ws,-mpt.x,-mpt.y);
					MoveWindow(GetDlgItem(hwndDlg,IDC_TABS),
						ws.left,ws.top,
						ws.right-ws.left,ws.bottom-ws.top+addheight,
						FALSE);
						
					// Move button IDOK
					GetWindowRect(GetDlgItem(hwndDlg,IDOK),&ws);
					OffsetRect(&ws,-mpt.x,-mpt.y);
					MoveWindow(GetDlgItem(hwndDlg,IDOK),
						ws.left,ws.top+addheight,
						ws.right-ws.left,ws.bottom-ws.top,
						FALSE);
					
					// Move button IDC_UPDATE
					GetWindowRect(GetDlgItem(hwndDlg,IDC_UPDATE),&ws);
					OffsetRect(&ws,-mpt.x,-mpt.y);
					MoveWindow(GetDlgItem(hwndDlg,IDC_UPDATE),
						ws.left,ws.top+addheight,
						ws.right-ws.left,ws.bottom-ws.top,
						FALSE);
					
					// Move button IDC_UPDATING
					GetWindowRect(GetDlgItem(hwndDlg,IDC_UPDATING),&ws);
					OffsetRect(&ws,-mpt.x,-mpt.y);
					MoveWindow(GetDlgItem(hwndDlg,IDC_UPDATING),
						ws.left,ws.top+addheight,
						ws.right-ws.left,ws.bottom-ws.top,
						FALSE);
				}
			}


			TabCtrl_SetCurSel(GetDlgItem(hwndDlg,IDC_TABS),dat->currentPage);
			dat->opd[dat->currentPage].hwnd=CreateDialogIndirectParam(dat->opd[dat->currentPage].hInst,dat->opd[dat->currentPage].pTemplate,hwndDlg,dat->opd[dat->currentPage].dlgProc,(LPARAM)dat->hContact);
			ThemeDialogBackground(dat->opd[dat->currentPage].hwnd);
			SetWindowPos(dat->opd[dat->currentPage].hwnd,HWND_TOP,dat->rcDisplay.left,dat->rcDisplay.top,0,0,SWP_NOSIZE);
			{	PSHNOTIFY pshn;
				pshn.hdr.code=PSN_INFOCHANGED;
				pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
				pshn.hdr.idFrom=0;
				pshn.lParam=(LPARAM)dat->hContact;
				SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);
			}
			ShowWindow(dat->opd[dat->currentPage].hwnd,SW_SHOW);
			dat->updateAnimFrame=0;
			GetDlgItemText(hwndDlg,IDC_UPDATING,dat->szUpdating,sizeof(dat->szUpdating));
			SendMessage(hwndDlg,M_CHECKONLINE,0,0);
			if(!IsWindowEnabled(GetDlgItem(hwndDlg,IDC_UPDATE)))
				ShowWindow(GetDlgItem(hwndDlg,IDC_UPDATING),SW_HIDE);
			else {
				EnableWindow(GetDlgItem(hwndDlg,IDC_UPDATE),FALSE);
				SetTimer(hwndDlg,1,100,NULL);
			}
			return TRUE;
		}
		case WM_TIMER:
		{	char str[128];
			sprintf(str,"%.*s%s%.*s",dat->updateAnimFrame%10,".........",dat->szUpdating,dat->updateAnimFrame%10,".........");
			SetDlgItemText(hwndDlg,IDC_UPDATING,str);
			if(++dat->updateAnimFrame==UPDATEANIMFRAMES) dat->updateAnimFrame=0;
			break;
		}
		case WM_CTLCOLORSTATIC:
			if(GetDlgItem(hwndDlg,IDC_WHITERECT)==(HWND)lParam) {
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
			else if(GetDlgItem(hwndDlg,IDC_UPDATING)==(HWND)lParam) {
				COLORREF textCol,bgCol,newCol;
				int ratio;
				textCol=GetSysColor(COLOR_BTNTEXT);
				bgCol=GetSysColor(COLOR_3DFACE);
				ratio=abs(UPDATEANIMFRAMES/2-dat->updateAnimFrame)*510/UPDATEANIMFRAMES;
				newCol=RGB(GetRValue(bgCol)+(GetRValue(textCol)-GetRValue(bgCol))*ratio/256,
						   GetGValue(bgCol)+(GetGValue(textCol)-GetGValue(bgCol))*ratio/256,
						   GetBValue(bgCol)+(GetBValue(textCol)-GetBValue(bgCol))*ratio/256);
				SetTextColor((HDC)wParam,newCol);
				SetBkColor((HDC)wParam,GetSysColor(COLOR_3DFACE));
				return (BOOL)GetSysColorBrush(COLOR_3DFACE);
			}
			SetBkMode((HDC)wParam,TRANSPARENT);
			return (BOOL)GetStockObject(NULL_BRUSH);
		case PSM_CHANGED:
			dat->opd[dat->currentPage].changed=1;
			return TRUE;
		case PSM_FORCECHANGED:
		{	PSHNOTIFY pshn;
			int i;

			pshn.hdr.code=PSN_INFOCHANGED;
			pshn.hdr.idFrom=0;
			pshn.lParam=(LPARAM)dat->hContact;
			for(i=0;i<dat->pageCount;i++) {
				pshn.hdr.hwndFrom=dat->opd[i].hwnd;
				if(dat->opd[i].hwnd!=NULL)
					SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);
			}
			break;
		}
		case M_CHECKONLINE:
		{
			char *szProto;
			if (dat->hContact != NULL) {
				szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)dat->hContact,0);
				if(szProto==NULL) {EnableWindow(GetDlgItem(hwndDlg,IDC_UPDATE),FALSE); break;}
				if(CallProtoService(szProto,PS_GETSTATUS,0,0)<ID_STATUS_ONLINE) EnableWindow(GetDlgItem(hwndDlg,IDC_UPDATE),FALSE);
				else EnableWindow(GetDlgItem(hwndDlg,IDC_UPDATE),!IsWindowVisible(GetDlgItem(hwndDlg,IDC_UPDATING)));
			}
			break;
		}
		case HM_PROTOACK:
		{	ACKDATA *ack=(ACKDATA*)lParam;
			int i;

			if(ack->hContact==NULL && ack->type==ACKTYPE_STATUS) {
				SendMessage(hwndDlg,M_CHECKONLINE,0,0);
				break;
			}
			if(ack->hContact!=dat->hContact) break;
			if(ack->type!=ACKTYPE_GETINFO) break;
			SendMessage(hwndDlg,PSM_FORCECHANGED,0,0);
			/* if they're not gonna send any more ACK's don't let that mean we should crash */
			if (!ack->hProcess && !ack->lParam) {
				ShowWindow(GetDlgItem(hwndDlg,IDC_UPDATING),SW_HIDE);
				KillTimer(hwndDlg,1);
				SendMessage(hwndDlg,M_CHECKONLINE,0,0);
				break;							
			} //if
			if(dat->infosUpdated==NULL) dat->infosUpdated=(int*)calloc(sizeof(int),(int)ack->hProcess);
			if(ack->result==ACKRESULT_SUCCESS || ack->result==ACKRESULT_FAILED) dat->infosUpdated[ack->lParam]=1;
			for(i=0;i<(int)ack->hProcess;i++)
				if(dat->infosUpdated[i]==0) break;
			if(i==(int)ack->hProcess) {
				ShowWindow(GetDlgItem(hwndDlg,IDC_UPDATING),SW_HIDE);
				KillTimer(hwndDlg,1);
				SendMessage(hwndDlg,M_CHECKONLINE,0,0);
			}
			break;
		}
		case WM_NOTIFY:
			switch(wParam) {
				case IDC_TABS:
					switch(((LPNMHDR)lParam)->code) {
						case TCN_SELCHANGING:
						{	PSHNOTIFY pshn;
							if(dat->currentPage==-1 || dat->opd[dat->currentPage].hwnd==NULL) break;
							pshn.hdr.code=PSN_KILLACTIVE;
							pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
							pshn.hdr.idFrom=0;
							pshn.lParam=(LPARAM)dat->hContact;
							if(SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn)) {
								SetWindowLong(hwndDlg,DWL_MSGRESULT,TRUE);
								return TRUE;
							}
							break;
						}
						case TCN_SELCHANGE:
							if(dat->currentPage!=-1 && dat->opd[dat->currentPage].hwnd!=NULL) ShowWindow(dat->opd[dat->currentPage].hwnd,SW_HIDE);
							dat->currentPage=TabCtrl_GetCurSel(GetDlgItem(hwndDlg,IDC_TABS));
							if(dat->currentPage!=-1) {
								if(dat->opd[dat->currentPage].hwnd==NULL) {
									PSHNOTIFY pshn;
									dat->opd[dat->currentPage].hwnd=CreateDialogIndirectParam(dat->opd[dat->currentPage].hInst,dat->opd[dat->currentPage].pTemplate,hwndDlg,dat->opd[dat->currentPage].dlgProc,(LPARAM)dat->hContact);
									ThemeDialogBackground(dat->opd[dat->currentPage].hwnd);
									SetWindowPos(dat->opd[dat->currentPage].hwnd,HWND_TOP,dat->rcDisplay.left,dat->rcDisplay.top,0,0,SWP_NOSIZE);
									pshn.hdr.code=PSN_INFOCHANGED;
									pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
									pshn.hdr.idFrom=0;
									pshn.lParam=(LPARAM)dat->hContact;
									SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);
								}
								ShowWindow(dat->opd[dat->currentPage].hwnd,SW_SHOW);
								SetFocus(GetDlgItem(hwndDlg,IDC_TABS));
							}
							break;
					}
					break;
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
				{	int i;
					PSHNOTIFY pshn;
					pshn.hdr.idFrom=0;
					pshn.lParam=(LPARAM)dat->hContact;
					pshn.hdr.code=PSN_RESET;
					for(i=0;i<dat->pageCount;i++) {
						if(dat->opd[i].hwnd==NULL || !dat->opd[i].changed) continue;
						pshn.hdr.hwndFrom=dat->opd[i].hwnd;
						SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn);
					}
					DestroyWindow(hwndDlg);
					break;
				}
				case IDOK:
				{	int i;
					PSHNOTIFY pshn;
					pshn.hdr.idFrom=0;
					pshn.lParam=(LPARAM)dat->hContact;
					if(dat->currentPage!=-1) {
						pshn.hdr.code=PSN_KILLACTIVE;
						pshn.hdr.hwndFrom=dat->opd[dat->currentPage].hwnd;
						if(SendMessage(dat->opd[dat->currentPage].hwnd,WM_NOTIFY,0,(LPARAM)&pshn))
							break;
					}

					pshn.hdr.code=PSN_APPLY;
					for(i=0;i<dat->pageCount;i++) {
						if(dat->opd[i].hwnd==NULL || !dat->opd[i].changed) continue;
						pshn.hdr.hwndFrom=dat->opd[i].hwnd;
						if(SendMessage(dat->opd[i].hwnd,WM_NOTIFY,0,(LPARAM)&pshn)==PSNRET_INVALID_NOCHANGEPAGE) {
							TabCtrl_SetCurSel(GetDlgItem(hwndDlg,IDC_TABS),i);
							if(dat->currentPage!=-1) ShowWindow(dat->opd[dat->currentPage].hwnd,SW_HIDE);
							dat->currentPage=i;
							ShowWindow(dat->opd[dat->currentPage].hwnd,SW_SHOW);
							return 0;
						}
					}
					DestroyWindow(hwndDlg);
					break;
				}
				case IDC_UPDATE:
					if(dat->infosUpdated!=NULL) {free(dat->infosUpdated); dat->infosUpdated=NULL;}
					if(dat->hContact != NULL) {
						CallContactService(dat->hContact,PSS_GETINFO,0,0);
						EnableWindow(GetDlgItem(hwndDlg,IDC_UPDATE),FALSE);
						ShowWindow(GetDlgItem(hwndDlg,IDC_UPDATING),SW_SHOW);
						SetTimer(hwndDlg,1,100,NULL);
					}
					break;
			}
			break;
		case WM_CLOSE:
			SendMessage(hwndDlg,WM_COMMAND,MAKEWPARAM(IDOK,BN_CLICKED),(LPARAM)GetDlgItem(hwndDlg,IDOK));
			break;
		case WM_DESTROY:
			{	TCITEM tci;
				char name[128];
				tci.mask=TCIF_TEXT;
				tci.pszText=name;
				tci.cchTextMax=sizeof(name);
				TabCtrl_GetItem(GetDlgItem(hwndDlg,IDC_TABS),dat->currentPage,&tci);
				DBWriteContactSettingString(NULL,"UserInfo","LastTab",name);
			}
			SendDlgItemMessage(hwndDlg,IDC_NAME,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDC_WHITERECT,WM_GETFONT,0,0),0);
			DeleteObject(dat->hBoldFont);
			WindowList_Remove(hWindowList,hwndDlg);
			UnhookEvent(dat->hProtoAckEvent);
			{	int i;
				for(i=0;i<dat->pageCount;i++)
					if(dat->opd[i].hwnd!=NULL) DestroyWindow(dat->opd[i].hwnd);
			}
			if(dat->infosUpdated!=NULL) free(dat->infosUpdated);
			free(dat->opd);
			free(dat);
			break;
	}
	return FALSE;
}

int ShutdownUserInfo(WPARAM wParam,LPARAM lParam)
{
	WindowList_BroadcastAsync(hWindowList,WM_DESTROY,0,0);
	return 0;
}

int LoadUserInfoModule(void)
{
	CLISTMENUITEM mi;

	CreateServiceFunction(MS_USERINFO_SHOWDIALOG,ShowDetailsDialogCommand);
	hDetailsInitEvent=CreateHookableEvent(ME_USERINFO_INITIALISE);
	HookEvent(ME_USERINFO_INITIALISE,DetailsInit);
	HookEvent(ME_DB_CONTACT_DELETED,UserInfoContactDelete);
	HookEvent(ME_SYSTEM_PRESHUTDOWN,ShutdownUserInfo);
	CreateServiceFunction(MS_USERINFO_ADDPAGE,AddDetailsPage);
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=1000050000;
	mi.flags=0;
	mi.hIcon=LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_USERDETAILS));
	mi.pszContactOwner=NULL;
	mi.pszName=Translate("User &Details");
	mi.pszService=MS_USERINFO_SHOWDIALOG;
	CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);
	mi.position=500050000;
	mi.pszName=Translate("View/Change My &Details...");
	CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);
	hWindowList=(HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST,0,0);
	return 0;
}
