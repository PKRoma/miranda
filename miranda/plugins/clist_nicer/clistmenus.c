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
//#include "CLUIFrames/genmenu.h"
#include "m_genmenu.h"
#include "m_ignore.h"
#include "CLUIFrames/cluiframes.h"

#pragma hdrstop

#define ME_CLIST_PREBUILDSTATUSMENU "CList/PreBuildStatusMenu"
#define MS_CLIST_ADDSTATUSMENUITEM "CList/AddStatusMenuItem"

typedef struct {
	WORD id;
	int iconId;
	CLISTMENUITEM mi;
}CListIntMenuItem,*lpCListIntMenuItem;

protoMenu *protoMenus = NULL;

//new menu sys
static int hStatusMenuObject;
int MenuModulesLoaded(WPARAM,LPARAM);

int statustopos(int status);

HANDLE hPreBuildStatusMenuEvent,hAckHook,hAckHook;

static HMENU hStatusMenu = 0;
int currentStatusMenuItem,currentDesiredStatusMode;
static int statusModeList[]={ID_STATUS_OFFLINE,ID_STATUS_ONLINE,ID_STATUS_AWAY,ID_STATUS_NA,ID_STATUS_OCCUPIED,ID_STATUS_DND,ID_STATUS_FREECHAT,ID_STATUS_INVISIBLE,ID_STATUS_ONTHEPHONE,ID_STATUS_OUTTOLUNCH};
static int skinIconStatusList[]={SKINICON_STATUS_OFFLINE,SKINICON_STATUS_ONLINE,SKINICON_STATUS_AWAY,SKINICON_STATUS_NA,SKINICON_STATUS_OCCUPIED,SKINICON_STATUS_DND,SKINICON_STATUS_FREE4CHAT,SKINICON_STATUS_INVISIBLE,SKINICON_STATUS_ONTHEPHONE,SKINICON_STATUS_OUTTOLUNCH};
static int statusModePf2List[]={0xFFFFFFFF,PF2_ONLINE,PF2_SHORTAWAY,PF2_LONGAWAY,PF2_LIGHTDND,PF2_HEAVYDND,PF2_FREECHAT,PF2_INVISIBLE,PF2_ONTHEPHONE,PF2_OUTTOLUNCH};

HANDLE *hStatusMainMenuHandles;
int hStatusMainMenuHandlesCnt;
typedef struct {
	int protoindex;
	int protostatus[sizeof(statusModeList)];
	int menuhandle[sizeof(statusModeList)];
}tStatusMenuHandles,*lpStatusMenuHandles;

lpStatusMenuHandles hStatusMenuHandles;
int hStatusMenuHandlesCnt;

extern HANDLE hStatusModeChangeEvent;
extern int  CheckProtocolOrder();
extern int g_shutDown;
extern struct ClcData *g_clcData;

//contactmenu exec param(ownerdata)
//also used in checkservice

typedef struct {
	char *proto;
	int protoindex;
	int status;

	BOOL custom;
	char *svc;
	int hMenuItem;
}StatusMenuExecParam,*lpStatusMenuExecParam;

int StatusMenuExecService(WPARAM wParam,LPARAM lParam)
{
	lpStatusMenuExecParam smep=(lpStatusMenuExecParam)wParam;
	if (smep != NULL && !IsBadReadPtr(smep, sizeof(StatusMenuExecParam))) {
		if (smep->custom) {
			if (smep->svc && *smep->svc)
				CallService(smep->svc, 0, (LPARAM)smep->hMenuItem);
		} else {
			PROTOCOLDESCRIPTOR **proto;
			int protoCount;
			int i;

			CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
			if ((smep->status==0) && (smep->protoindex!=0) && (smep->proto!=NULL)) {
				PMO_IntMenuItem pimi;
				int i=(DBGetContactSettingByte(NULL,smep->proto,"LockMainStatus",0)?0:1);
				DBWriteContactSettingByte(NULL,smep->proto,"LockMainStatus",i);
				pimi = pcli->pfnMOGetIntMenuItem(smep->protoindex);
				if (i)
					pimi->mi.flags|=CMIF_CHECKED;
				else pimi->mi.flags&=~CMIF_CHECKED;
				if(pcli->hwndStatus)
					InvalidateRect(pcli->hwndStatus, NULL, FALSE);
			}
			else if ((smep->proto!=NULL)) {
				CallProtoService(smep->proto,PS_SETSTATUS,smep->status,0);  
				NotifyEventHooks(hStatusModeChangeEvent, smep->status, (LPARAM)smep->proto);
				//CallProtoService(smep->proto,PS_SETSTATUS,smep->status,0);	
			}
			else {
				int MenusProtoCount=0;
				for (i=0;i<protoCount;i++)
					MenusProtoCount+=GetProtocolVisibility(proto[i]->szName)?1:0;

				currentDesiredStatusMode = smep->status;
				//	NotifyEventHooks(hStatusModeChangeEvent,currentDesiredStatusMode,0);
				for (i=0;i<protoCount;i++) {
					if (!(MenusProtoCount>1 && DBGetContactSettingByte(NULL,proto[i]->szName,"LockMainStatus",0)))
						CallProtoService(proto[i]->szName,PS_SETSTATUS,currentDesiredStatusMode,0);
				}
				NotifyEventHooks(hStatusModeChangeEvent,currentDesiredStatusMode,0);

				DBWriteContactSettingWord(NULL,"CList","Status",(WORD)currentDesiredStatusMode);
				return 1;  
			}
		}
	}
	return(0);
}

int FreeOwnerDataStatusMenu (WPARAM wParam,LPARAM lParam)
{
	lpStatusMenuExecParam smep = (lpStatusMenuExecParam)lParam;
	if ( smep != NULL) {
		if (smep->proto)
			FreeAndNil(&smep->proto);
		if (smep->svc)
			FreeAndNil(&smep->svc);
		FreeAndNil(&smep);
	}

	return(0);
}

static int BuildStatusMenu(WPARAM wParam,LPARAM lParam)
{
	int tick;
	HMENU hMenu;
	ListParam param;

	memset(&param,0,sizeof(param));
	param.MenuObjectHandle=hStatusMenuObject;
	param.rootlevel=-1;

	hMenu=hStatusMenu;
	//clear statusmenu
	while ( GetMenuItemCount( hStatusMenu ) > 0 )
		DeleteMenu( hStatusMenu, 0, MF_BYPOSITION );

	tick=GetTickCount();

	CallService(MO_BUILDMENU,(WPARAM)hMenu,(LPARAM)&param);
	DrawMenuBar((HWND)CallService("CLUI/GetHwnd",(WPARAM)0,(LPARAM)0));
	tick=GetTickCount()-tick;
	return(int)hMenu;
}

int GetProtoIndexByPos(PROTOCOLDESCRIPTOR ** proto, int protoCnt, int Pos)
{
	int res=0;
	int p;
	char buf[10];
	char *b2 = NULL;

	_itoa(Pos,buf,10);
	b2=DBGetString(NULL,"Protocols",buf);
	//TRACE("GetProtoIndexByPos \r\n");

	if (b2) {
		for (p=0; p<protoCnt; p++) {
			if (strcmp(proto[p]->szName,b2)==0) {
				mir_free(b2);       // XXX
				return p;
			}
		}
		mir_free(b2);               // XXX 
	}
	return -1;
}

int MenuModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	int i,j,protoCount=0,networkProtoCount,s;
	int storedProtoCount;
	PROTOCOLDESCRIPTOR **proto;
	DWORD statusFlags=0,flags,moreflags;
	TMO_MenuItem tmi;
	TMenuParam tmp;
	HANDLE mi;
	int pos=0;
	int vis = 0;

	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
	networkProtoCount=0;
	CheckProtocolOrder();

	//clear statusmenu
	while (GetMenuItemCount(hStatusMenu)>0)
		DeleteMenu(hStatusMenu,0,MF_BYPOSITION);

	//status menu
	if (hStatusMenuObject!=0) {
		CallService(MO_REMOVEMENUOBJECT,hStatusMenuObject,0);
		if (hStatusMainMenuHandles!=NULL)
			mir_free(hStatusMainMenuHandles);
		if (hStatusMenuHandles!=NULL)
			mir_free(hStatusMenuHandles);
	}

	memset(&tmp,0,sizeof(tmp));
	tmp.cbSize=sizeof(tmp);
	tmp.CheckService=NULL;
	tmp.ExecService="StatusMenuExecService";
	tmp.name="StatusMenu";

	hStatusMenuObject=(int)CallService(MO_CREATENEWMENUOBJECT,(WPARAM)0,(LPARAM)&tmp);
	{
		OptParam op;
		op.Handle=hStatusMenuObject;
		op.Setting=OPT_MENUOBJECT_SET_FREE_SERVICE;
		op.Value=(int)"CLISTMENUS/FreeOwnerDataStatusMenu";
		CallService(MO_SETOPTIONSMENUOBJECT,(WPARAM)0,(LPARAM)&op);

		op.Handle=hStatusMenuObject;
		op.Setting=OPT_USERDEFINEDITEMS;
		op.Value=(int)TRUE;
		CallService(MO_SETOPTIONSMENUOBJECT,(WPARAM)0,(LPARAM)&op);
	}

	hStatusMainMenuHandles=(HANDLE*)mir_alloc(sizeof(statusModeList));
	hStatusMainMenuHandlesCnt=sizeof(statusModeList)/sizeof(HANDLE);
	for (i=0;i<protoCount;i++)
		if (proto[i]->type==PROTOTYPE_PROTOCOL && (DBGetContactSettingByte(NULL,"CLUI","DontHideStatusMenu",0)||GetProtocolVisibility(proto[i]->szName)!=0))
			networkProtoCount++;

	memset(hStatusMainMenuHandles,0,sizeof(statusModeList));
	hStatusMenuHandles=(tStatusMenuHandles*)mir_alloc(sizeof(tStatusMenuHandles)*protoCount);
	hStatusMenuHandlesCnt=protoCount;

	memset(hStatusMenuHandles,0,sizeof(tStatusMenuHandles)*protoCount);
	storedProtoCount=DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);

	if(protoMenus)
		free(protoMenus);

	protoMenus = malloc(sizeof(protoMenu) * (storedProtoCount + 2));
	ZeroMemory(protoMenus, sizeof(protoMenu) * (storedProtoCount + 2));

	for (s=0;s<storedProtoCount;s++) {
		pos=0;
		i=GetProtoIndexByPos(proto,protoCount,s);
		if (i==-1) continue;
		if ((proto[i]->type!=PROTOTYPE_PROTOCOL) || (DBGetContactSettingByte(NULL,"CLUI","DontHideStatusMenu",0)==0&&GetProtocolVisibility(proto[i]->szName)==0)) continue;

		flags=CallProtoService(proto[i]->szName,PS_GETCAPS,PFLAGNUM_2,0);
		moreflags = CallProtoService(proto[i]->szName,PS_GETCAPS, PFLAGNUM_5, 0);
		if (networkProtoCount > 0) {
			char protoName[128];
			int j;
			int rootmenu;
			HICON ic;
			//adding root
			memset(&tmi,0,sizeof(tmi));
			memset(protoName,0,128);
			tmi.cbSize=sizeof(tmi);
			tmi.flags=CMIF_CHILDPOPUP|CMIF_ROOTPOPUP;
			tmi.position=pos++;
			tmi.hIcon=(HICON)CallProtoService(proto[i]->szName,PS_LOADICON,PLI_PROTOCOL|PLIF_SMALL,0);
			ic=tmi.hIcon;
			tmi.root=-1;
			CallProtoService(proto[i]->szName,PS_GETNAME,sizeof(protoName),(LPARAM)protoName);
			tmi.pszName = protoName;
			rootmenu=CallService(MO_ADDNEWMENUITEM,(WPARAM)hStatusMenuObject,(LPARAM)&tmi);
			strncpy(protoMenus[vis].protoName, protoName, 50);
			protoMenus[vis].menuID = rootmenu;
			protoMenus[vis].protoName[49] = 0;
			vis++;
			memset(&tmi,0,sizeof(tmi));
			tmi.cbSize=sizeof(tmi);
			tmi.flags=CMIF_CHILDPOPUP;
			//if(statusModeList[j]==ID_STATUS_OFFLINE){tmi.flags|=CMIF_CHECKED;}
			tmi.root=rootmenu;
			tmi.position=pos++;
			tmi.pszName=protoName;
			tmi.hIcon=(HICON)CallProtoService(proto[i]->szName,PS_LOADICON,PLI_PROTOCOL|PLIF_SMALL,0);
			{
				lpStatusMenuExecParam smep;
				smep=(lpStatusMenuExecParam)mir_alloc(sizeof(StatusMenuExecParam));
				memset(smep, 0, sizeof(StatusMenuExecParam));
				smep->status=0;
				smep->protoindex=0;
				smep->proto=mir_strdup(proto[i]->szName);
				tmi.ownerdata=smep;
				if (DBGetContactSettingByte(NULL,smep->proto,"LockMainStatus",0)) tmi.flags|=CMIF_CHECKED;
			}
			mi=(HANDLE)CallService(MO_ADDNEWMENUITEM,(WPARAM)hStatusMenuObject,(LPARAM)&tmi);
			((lpStatusMenuExecParam)tmi.ownerdata)->protoindex=(int)mi;
			CallService(MO_MODIFYMENUITEM,(WPARAM)mi,(LPARAM)&tmi);
			pos=+100000;
			for (j=0;j<sizeof(statusModeList)/sizeof(statusModeList[0]);j++) {
				if (!(flags&statusModePf2List[j]) || (moreflags & statusModePf2List[j] && j > 0))
					continue;
				else {
					//adding
					memset(&tmi,0,sizeof(tmi));
					tmi.cbSize=sizeof(tmi);
					tmi.flags=CMIF_CHILDPOPUP;
					if (statusModeList[j]==ID_STATUS_OFFLINE)
						tmi.flags|=CMIF_CHECKED;

					tmi.root=rootmenu;
					tmi.position=pos++;
					tmi.pszName=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,statusModeList[j],0);
					tmi.hIcon=LoadSkinnedProtoIcon(proto[i]->szName,statusModeList[j]);
					{
						lpStatusMenuExecParam smep;
						smep=(lpStatusMenuExecParam)mir_alloc(sizeof(StatusMenuExecParam));
						memset(smep, 0, sizeof(StatusMenuExecParam));
						smep->status=statusModeList[j];
						smep->protoindex=s;
						smep->proto=mir_strdup(proto[i]->szName);
						tmi.ownerdata=smep;
					}
					hStatusMenuHandles[i].protoindex=i;
					hStatusMenuHandles[i].protostatus[j]=statusModeList[j];                 
					hStatusMenuHandles[i].menuhandle[j]=CallService(MO_ADDNEWMENUITEM,(WPARAM)hStatusMenuObject,(LPARAM)&tmi);
				}
			}
		}
		statusFlags |= flags & ~moreflags;
	}
	if (networkProtoCount > 0) {
		pos+=100000;
	}
	else {
		strncpy(protoMenus[0].protoName, proto[0]->szName, 50);
		protoMenus[0].menuID = -1;
		protoMenus[0].protoName[49] = 0;
	}
	//NotifyEventHooks(hPreBuildStatusMenuEvent, 0, 0);
	{
		for (j = 0; j<sizeof(statusModeList)/sizeof(statusModeList[0]);j++) {
			for (i=0;i<protoCount;i++) {
				if (proto[i]->type!=PROTOTYPE_PROTOCOL || (DBGetContactSettingByte(NULL,"CLUI","DontHideStatusMenu",0)==0&&GetProtocolVisibility(proto[i]->szName)==0)) continue;
				flags=CallProtoService(proto[i]->szName,PS_GETCAPS,PFLAGNUM_2,0);
				if (flags&statusModePf2List[j]) {
					//DeleteMenu(hMenu,statusModeList[j],MF_BYCOMMAND)
					int buf[256];
					memset(&tmi,0,sizeof(tmi));
					memset(&buf,0,256);
					tmi.cbSize=sizeof(tmi);
					tmi.flags=CMIF_CHILDPOPUP;
					if (statusModeList[j]==ID_STATUS_OFFLINE) {
						tmi.flags|=CMIF_CHECKED;
					}
					tmi.hIcon=LoadSkinnedIcon(skinIconStatusList[j]);
					tmi.position=pos++;
					tmi.root=-1;
					tmi.hotKey=MAKELPARAM(MOD_CONTROL,'0'+j);
					tmi.pszName=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,statusModeList[j],0);
					wsprintfA(( LPSTR )buf, "%s\tCtrl+%c", tmi.pszName, '0'+j );
					tmi.pszName=(char*)buf;
					{
						//owner data
						lpStatusMenuExecParam smep;
						smep=(lpStatusMenuExecParam)mir_alloc(sizeof(StatusMenuExecParam));
						memset(smep, 0, sizeof(StatusMenuExecParam));
						smep->status=statusModeList[j];
						smep->proto=NULL;
						tmi.ownerdata=smep;

					}                              
					hStatusMainMenuHandles[j]=(void **)(CallService(MO_ADDNEWMENUITEM,(WPARAM)hStatusMenuObject,(LPARAM)&tmi));
					break;
	}	}	}	}

	BuildStatusMenu(0,0);
	return 0;
}

int statustopos(int status)
{
	int j;
	if (statusModeList==NULL) return(-1);
	for (j=0;j<sizeof(statusModeList)/sizeof(statusModeList[0]);j++) {
		if (status==statusModeList[j]) {
			return(j);
		}
	}
	return(-1);
}

static int MenuProtoAck(WPARAM wParam,LPARAM lParam)
{
	int protoCount,i,networkProtoCount;
	PROTOCOLDESCRIPTOR **proto;
	ACKDATA *ack=(ACKDATA*)lParam;
	int overallStatus=0,thisStatus;
	TMO_MenuItem tmi;

	if (ack->type!=ACKTYPE_STATUS) return 0;
	if (ack->result!=ACKRESULT_SUCCESS) return 0;
	if (hStatusMainMenuHandles==NULL) return(0);
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
	networkProtoCount=0;
	for (i=0;i<protoCount;i++)
		if (proto[i]->type==PROTOTYPE_PROTOCOL) {
			thisStatus=CallProtoService(proto[i]->szName,PS_GETSTATUS,0,0);
			if (overallStatus==0) overallStatus=thisStatus;
			else if (overallStatus!=thisStatus) overallStatus=-1;
			networkProtoCount++;
		}
		memset(&tmi,0,sizeof(tmi));
		tmi.cbSize=sizeof(tmi);
		if (overallStatus>ID_STATUS_CONNECTING) {
			//tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
			//CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMainMenuHandles[statustopos(currentStatusMenuItem)],(LPARAM)&tmi);
			int pos = statustopos(currentStatusMenuItem);
			if (pos>=0 && pos < hStatusMainMenuHandlesCnt) {
				tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
				CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMainMenuHandles[pos],(LPARAM)&tmi);
			}      

			currentStatusMenuItem=overallStatus;
			pos = statustopos(currentStatusMenuItem);
			//tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|CMIF_CHECKED; 
			//CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMainMenuHandles[statustopos(currentStatusMenuItem)],(LPARAM)&tmi);				
			if (pos>=0 && pos < hStatusMainMenuHandlesCnt) {
				tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|CMIF_CHECKED;
				CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMainMenuHandles[pos],(LPARAM)&tmi);
			}      
			currentDesiredStatusMode=currentStatusMenuItem;
		}
		else {
			int pos = statustopos(currentStatusMenuItem);
			if (pos>=0 && pos < hStatusMainMenuHandlesCnt) {
				tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
				CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMainMenuHandles[pos],(LPARAM)&tmi);
			}              
			//SetMenuDefaultItem(hStatusMenu,-1,FALSE);
			currentStatusMenuItem=0;
		}
		if (networkProtoCount<=1) return 0;
		for (i=0;i<protoCount;i++)
			if (!strcmp(proto[i]->szName,ack->szModule)) break;
		//hProcess is previous mode, lParam is new mode
		//hProcess is previous mode, lParam is new mode
		if ((int)ack->hProcess>=ID_STATUS_OFFLINE && (int)ack->hProcess<ID_STATUS_OFFLINE+sizeof(statusModeList)/sizeof(statusModeList[0])) {
			int pos = statustopos((int)ack->hProcess);
			if (pos>=0 && pos < sizeof(statusModeList)/sizeof(statusModeList[0])) {
				tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
				CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMenuHandles[i].menuhandle[pos],(LPARAM)&tmi);
			}
		}

		if (ack->lParam>=ID_STATUS_OFFLINE && ack->lParam<ID_STATUS_OFFLINE+sizeof(statusModeList)/sizeof(statusModeList[0])) {
			int pos = statustopos((int)ack->lParam);
			if (pos>=0 && pos < sizeof(statusModeList)/sizeof(statusModeList[0])) {
				tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|CMIF_CHECKED;
				CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMenuHandles[i].menuhandle[pos],(LPARAM)&tmi);
			}

		}
		BuildStatusMenu(0,0);
		return 0;
}

static int AddStatusMenuItem(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM *mi=(CLISTMENUITEM*)lParam;
	TMO_MenuItem tmi;
	OptParam op;
	int i;
	char extractedProtoName[50], *szFound = NULL;
	lpStatusMenuExecParam smep;

	if(mi->cbSize!=sizeof(CLISTMENUITEM) || protoMenus == NULL) 
		return 0;

	if(mi->pszPopupName == NULL)
		goto no_custom_status_item;

	strncpy(extractedProtoName, mi->pszPopupName, 50);
	extractedProtoName[49] = 0;
	szFound = strstr(&extractedProtoName[2], "Custom Status");
	if(szFound) {
		*(szFound - 1) = 0;
	}
	else
		goto no_custom_status_item;

	for(i = 0; protoMenus[i].protoName[0]; i++) {
		if(!strcmp(protoMenus[i].protoName, extractedProtoName)) {
			if(protoMenus[i].added)
				break;
			else {
				TMO_MenuItem tmi = {0};

				protoMenus[i].added = TRUE;

				tmi.cbSize=sizeof(tmi);
				tmi.flags = CMIF_CHILDPOPUP | CMIF_ROOTPOPUP;
				tmi.hIcon=mi->hIcon;
				tmi.hotKey=mi->hotKey;
				tmi.position = 1;
				tmi.pszName = "Custom Status"; //mi->pszName;
				tmi.root = protoMenus[i].menuID;
				protoMenus[i].menuID = (int)CallService(MO_ADDNEWMENUITEM,(WPARAM)hStatusMenuObject,(LPARAM)&tmi);
				break;
			}
		}
	}
	if(protoMenus[i].protoName[0] == 0)
		return 0;

no_custom_status_item:

	memset(&tmi,0,sizeof(tmi));
	tmi.cbSize=sizeof(tmi);
	tmi.flags = (szFound ? CMIF_CHILDPOPUP : 0) | mi->flags;
	tmi.hIcon=mi->hIcon;
	tmi.hotKey=mi->hotKey;
	tmi.position=mi->position;
	tmi.pszName = mi->pszName;
	tmi.root = szFound ? (int)protoMenus[i].menuID : (int)mi->pszPopupName;

	//owner data
	smep=(lpStatusMenuExecParam)mir_alloc(sizeof(StatusMenuExecParam));
	memset(smep,0,sizeof(StatusMenuExecParam));
	smep->custom = TRUE;
	smep->svc=mir_strdup(mi->pszService);
	tmi.ownerdata=smep;

	op.Handle=(int)CallService(MO_ADDNEWMENUITEM,(WPARAM)hStatusMenuObject,(LPARAM)&tmi);  
	((lpStatusMenuExecParam)(tmi.ownerdata))->hMenuItem=op.Handle;
	op.Setting=OPT_MENUITEMSETUNIQNAME;

	{
		char buf[256];
		mir_snprintf(buf, sizeof(buf), "%s/%s", mi->pszPopupName?mi->pszPopupName:"",mi->pszService?mi->pszService:"");
		op.Value=(int)buf;
		CallService(MO_SETOPTIONSMENUITEM,(WPARAM)0,(LPARAM)&op);
	}
	return(op.Handle);
}

static int MenuModulesShutdown(WPARAM wParam,LPARAM lParam)
{
	UnhookEvent(hAckHook);
	if(protoMenus) {
		free(protoMenus);
		protoMenus = NULL;
	}
	return 0;
}

int CloseAction(WPARAM wParam,LPARAM lParam)
{
	int k;
	g_shutDown = 1;
	k=CallService(MS_SYSTEM_OKTOEXIT,(WPARAM)0,(LPARAM)0);
	if (k) {
		SendMessage( pcli->hwndContactList, WM_DESTROY, 0, 0 );
		PostQuitMessage(0);
		Sleep(0);
	}

	return(0);
}

static HANDLE hWindowListIGN = 0;

/*                                                              
 * dialog procedure for handling the contact ignore dialog (available from the contact
 * menu
 */

static BOOL CALLBACK IgnoreDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)GetWindowLong(hWnd, GWL_USERDATA);

	switch(msg) {
	case WM_INITDIALOG:
		{
			DWORD dwMask;
			struct ClcContact *contact = NULL;
			int pCaps;
			HWND hwndAdd;

			hContact = (HANDLE)lParam;
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)hContact);
			dwMask = DBGetContactSettingDword(hContact, "Ignore", "Mask1", 0);
			SendMessage(hWnd, WM_USER + 100, (WPARAM)hContact, dwMask);
			SendMessage(hWnd, WM_USER + 120, 0, 0);
			TranslateDialogDefault(hWnd);
			hwndAdd = GetDlgItem(hWnd, IDC_IGN_ADDPERMANENTLY); // CreateWindowEx(0, _T("CLCButtonClass"), _T("FOO"), WS_VISIBLE | BS_PUSHBUTTON | WS_CHILD | WS_TABSTOP, 200, 276, 106, 24, hWnd, (HMENU)IDC_IGN_ADDPERMANENTLY, g_hInst, NULL);
			SendMessage(hwndAdd, BUTTONSETASFLATBTN, 0, 1);
			SendMessage(hwndAdd, BUTTONSETASFLATBTN + 10, 0, 1);

			SendMessage(hwndAdd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(210), IMAGE_ICON, 16, 16, LR_SHARED));
			SetWindowText(hwndAdd, TranslateT("Add permanently"));
			EnableWindow(hwndAdd, DBGetContactSettingByte(hContact, "CList", "NotOnList", 0));

			if(g_clcData) {
				if(FindItem(pcli->hwndContactTree, g_clcData, hContact, &contact, NULL, NULL)) {
					if(contact && contact->type != CLCIT_CONTACT) {
						DestroyWindow(hWnd);
						return FALSE;
					} else if(contact) {
						TCHAR szTitle[512];

						mir_sntprintf(szTitle, 512, TranslateT("Ignore options for %s"), contact->szText);
						SetWindowText(hWnd, szTitle);
						SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
						pCaps = CallProtoService(contact->proto, PS_GETCAPS, PFLAGNUM_1, 0);
						EnableWindow(GetDlgItem(hWnd, IDC_IGN_ALWAYSONLINE), pCaps & PF1_INVISLIST ? TRUE : FALSE);
						EnableWindow(GetDlgItem(hWnd, IDC_IGN_ALWAYSOFFLINE), pCaps & PF1_VISLIST ? TRUE : FALSE);
					}
				} else {
					DestroyWindow(hWnd);
					return FALSE;
				}
			}
			WindowList_Add(hWindowListIGN, hWnd, hContact);
			ShowWindow(hWnd, SW_SHOWNORMAL);
			return TRUE;
		}
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
	  	case IDC_IGN_ALL:
			SendMessage(hWnd, WM_USER + 100, (WPARAM)hContact, (LPARAM)0xffffffff);
		  	return 0;
	  	case IDC_IGN_NONE:
			SendMessage(hWnd, WM_USER + 100, (WPARAM)hContact, (LPARAM)0);
		  	return 0;
	  	case IDC_IGN_ALWAYSONLINE:
			if(IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSONLINE))
				CheckDlgButton(hWnd, IDC_IGN_ALWAYSOFFLINE, FALSE);
		  	break;
	  	case IDC_IGN_ALWAYSOFFLINE:
			if(IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSOFFLINE))
				CheckDlgButton(hWnd, IDC_IGN_ALWAYSONLINE, FALSE);
		  	break;
	  	case IDC_HIDECONTACT:
			DBWriteContactSettingByte(hContact, "CList", "Hidden", IsDlgButtonChecked(hWnd, IDC_HIDECONTACT) ? 1 : 0);
		  	break;
	  	case IDC_IGN_ADDPERMANENTLY:
			{
				ADDCONTACTSTRUCT acs = {0};

				acs.handle = hContact;
			  	acs.handleType = HANDLE_CONTACT;
			  	acs.szProto = 0;
			  	CallService(MS_ADDCONTACT_SHOW, (WPARAM)hWnd, (LPARAM)&acs);
			  	EnableWindow(GetDlgItem(hWnd, IDC_IGN_ADDPERMANENTLY), DBGetContactSettingByte(hContact, "CList", "NotOnList", 0));
			  	break;
		  	}
	  	case IDOK:
			{
				DWORD newMask = 0;
			  	SendMessage(hWnd, WM_USER + 110, 0, (LPARAM)&newMask);
			  	DBWriteContactSettingDword(hContact, "Ignore", "Mask1", newMask);
			  	SendMessage(hWnd, WM_USER + 130, 0, 0);
		  	}
	  	case IDCANCEL:
			DestroyWindow(hWnd);
		  	break;
		}
		break;
	case WM_USER + 100:                                         // fill dialog (wParam = hContact, lParam = mask)
		{
			CheckDlgButton(hWnd, IDC_IGN_MSGEVENTS, lParam & (1 << (IGNOREEVENT_MESSAGE - 1)) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_IGN_FILEEVENTS, lParam & (1 << (IGNOREEVENT_FILE - 1)) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_IGN_URLEVENTS, lParam & (1 << (IGNOREEVENT_URL - 1)) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_IGN_AUTH, lParam & (1 << (IGNOREEVENT_AUTHORIZATION - 1)) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_IGN_ADD, lParam & (1 << (IGNOREEVENT_YOUWEREADDED - 1)) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_IGN_ONLINE, lParam & (1 << (IGNOREEVENT_USERONLINE - 1)) ? BST_CHECKED : BST_UNCHECKED);
			return 0;
		}
	case WM_USER + 110:                                         // retrieve value
		{
			DWORD *dwNewMask = (DWORD *)lParam, dwMask = 0;

			dwMask = (IsDlgButtonChecked(hWnd, IDC_IGN_MSGEVENTS) ? (1 << (IGNOREEVENT_MESSAGE - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_FILEEVENTS) ? (1 << (IGNOREEVENT_FILE - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_URLEVENTS) ? (1 << (IGNOREEVENT_URL - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_AUTH) ? (1 << (IGNOREEVENT_AUTHORIZATION - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_ADD) ? (1 << (IGNOREEVENT_YOUWEREADDED - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_ONLINE) ? (1 << (IGNOREEVENT_USERONLINE - 1)) : 0);

			if(dwNewMask)
				*dwNewMask = dwMask;
			return 0;
		}
	case WM_USER + 120:                                         // set visibility status
		{
			struct ClcContact *contact = NULL;

			if(FindItem(pcli->hwndContactTree, g_clcData, hContact, &contact, NULL, NULL)) {
				if(contact) {
					WORD wApparentMode = DBGetContactSettingWord(contact->hContact, contact->proto, "ApparentMode", 0);

					CheckDlgButton(hWnd, IDC_IGN_ALWAYSOFFLINE, wApparentMode == ID_STATUS_OFFLINE ? TRUE : FALSE);
					CheckDlgButton(hWnd, IDC_IGN_ALWAYSONLINE, wApparentMode == ID_STATUS_ONLINE ? TRUE : FALSE);
				}
			}
			return 0;
		}
	case WM_USER + 130:                                         // update apparent mode
		{
			struct ClcContact *contact = NULL;

			if(FindItem(pcli->hwndContactTree, g_clcData, hContact, &contact, NULL, NULL)) {
				if(contact) {
					WORD wApparentMode = 0, oldApparentMode = DBGetContactSettingWord(hContact, contact->proto, "ApparentMode", 0);

					if(IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSONLINE))
						wApparentMode = ID_STATUS_ONLINE;
					else if(IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSOFFLINE))
						wApparentMode = ID_STATUS_OFFLINE;

					//DBWriteContactSettingWord(hContact, contact->proto, "ApparentMode", wApparentMode);
					//if(oldApparentMode != wApparentMode)
					CallContactService(hContact, PSS_SETAPPARENTMODE, (WPARAM)wApparentMode, 0);
					SendMessage(hWnd, WM_USER + 120, 0, 0);
				}
			}
			return 0;
		}
	case WM_DESTROY:
		SetWindowLong(hWnd, GWL_USERDATA, 0);
		WindowList_Remove(hWindowListIGN, hWnd);
		break;
	}
	return FALSE;
}

/*                                                              
 * service function: Open ignore settings dialog for the contact handle in wParam
 * (clist_nicer+ specific service)
 * 
 * Servicename = CList/SetContactIgnore
 *
 * ensure that dialog is only opened once (the dialog proc saves the window handle of an open dialog 
 * of this type to the contacts database record).
 *
 * if dialog is already open, focus it.
*/

static int SetContactIgnore(WPARAM wParam, LPARAM lParam)
{
	HANDLE hWnd = 0;

	if(hWindowListIGN == 0)
		hWindowListIGN = (HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);

	hWnd = WindowList_Find(hWindowListIGN, (HANDLE)wParam);

	if(wParam) {
		if(hWnd == 0)
			CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_QUICKIGNORE), 0, IgnoreDialogProc, (LPARAM)wParam);
		else {
			if(IsWindow(hWnd))
				SetFocus(hWnd);
		}
	}
	return 0;
}

/*                                                              
 * service function: Set or clear a contacts priority flag (this is a toggle service)
 * (clist_nicer+ specific service)
 * 
 * Servicename = CList/SetContactPriority (wParam = contacts handle)
 *
 * priority contacts appear on top of the current sorting order (the priority flag
 * overrides any other sorting, thus putting priority contacts at the top of the list
 * or group). If more than one contact per group have this flag set, they will be
 * sorted using the current contact list sorting rule(s).
*/

static int SetContactPriority(WPARAM wParam, LPARAM lParam)
{
	SendMessage(pcli->hwndContactTree, CLM_TOGGLEPRIORITYCONTACT, wParam, lParam);
	return 0;
}

/*                                                              
 * service function: Set a contacts floating status.
 * (clist_nicer+ specific service)
 * 
 * Servicename = CList/SetContactFloating
 *
 * a floating contact appears as a small independent top level window anywhere on
 * the desktop.
*/

static int SetContactFloating(WPARAM wParam, LPARAM lParam)
{
	SendMessage(pcli->hwndContactTree, CLM_TOGGLEFLOATINGCONTACT, wParam, lParam);
	return 0;
}

int InitCustomMenus(void)
{
	CreateServiceFunction("StatusMenuExecService",StatusMenuExecService);

	CreateServiceFunction("CloseAction",CloseAction);
	CreateServiceFunction("CList/SetContactPriority", SetContactPriority);
	CreateServiceFunction("CList/SetContactFloating", SetContactFloating);
	CreateServiceFunction("CList/SetContactIgnore", SetContactIgnore);

	//free services
	CreateServiceFunction("CLISTMENUS/FreeOwnerDataStatusMenu",FreeOwnerDataStatusMenu);

	CreateServiceFunction(MS_CLIST_ADDSTATUSMENUITEM,AddStatusMenuItem);
	hPreBuildStatusMenuEvent=CreateHookableEvent(ME_CLIST_PREBUILDSTATUSMENU);

	hAckHook=(HANDLE)HookEvent(ME_PROTO_ACK,MenuProtoAck);

	hStatusMenu=GetSubMenu(LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(IDR_CLISTMENU)),1);
	CallService(MS_LANGPACK_TRANSLATEMENU,(WPARAM)hStatusMenu,0);

	hStatusMainMenuHandles=NULL;
	hStatusMainMenuHandlesCnt=0;

	hStatusMenuHandles=NULL;
	hStatusMenuHandlesCnt=0;

	currentStatusMenuItem=ID_STATUS_OFFLINE;
	currentDesiredStatusMode=ID_STATUS_OFFLINE;

	HookEvent(ME_SYSTEM_SHUTDOWN,MenuModulesShutdown);
	return 0;
}

void UninitCustomMenus(void)
{
	if ( hStatusMainMenuHandles != NULL )
		mir_free(hStatusMainMenuHandles);
	hStatusMainMenuHandles=NULL;

	if ( hStatusMenuHandles != NULL )
		mir_free(hStatusMenuHandles);

	hStatusMenuHandles=NULL;
}
