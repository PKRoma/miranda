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
#include "clist.h"
#include "m_genmenu.h"
#include "m_clui.h"
#pragma hdrstop


#define FIRSTCUSTOMMENUITEMID	30000
#define MENU_CUSTOMITEMMAIN		0x80000000
//#define MENU_CUSTOMITEMCONTEXT	0x40000000
//#define MENU_CUSTOMITEMFRAME	0x20000000

typedef struct  {
	WORD id;
	int iconId;
	CLISTMENUITEM mi;
}CListIntMenuItem,*lpCListIntMenuItem;

//new menu sys
static int hStatusMenuObject;
int MenuModulesLoaded(WPARAM,LPARAM);

int statustopos(int status);

static HMENU hStatusMenu = 0;
int currentStatusMenuItem,currentDesiredStatusMode;
static int statusModeList[]={ID_STATUS_OFFLINE,ID_STATUS_ONLINE,ID_STATUS_AWAY,ID_STATUS_NA,ID_STATUS_OCCUPIED,ID_STATUS_DND,ID_STATUS_FREECHAT,ID_STATUS_INVISIBLE,ID_STATUS_ONTHEPHONE,ID_STATUS_OUTTOLUNCH};
static int skinIconStatusList[]={SKINICON_STATUS_OFFLINE,SKINICON_STATUS_ONLINE,SKINICON_STATUS_AWAY,SKINICON_STATUS_NA,SKINICON_STATUS_OCCUPIED,SKINICON_STATUS_DND,SKINICON_STATUS_FREE4CHAT,SKINICON_STATUS_INVISIBLE,SKINICON_STATUS_ONTHEPHONE,SKINICON_STATUS_OUTTOLUNCH};
static int statusModePf2List[]={0xFFFFFFFF,PF2_ONLINE,PF2_SHORTAWAY,PF2_LONGAWAY,PF2_LIGHTDND,PF2_HEAVYDND,PF2_FREECHAT,PF2_INVISIBLE,PF2_ONTHEPHONE,PF2_OUTTOLUNCH};

HANDLE *hStatusMainMenuHandles;
int hStatusMainMenuHandlesCnt;
typedef struct
{
	int protoindex;
	int protostatus[sizeof(statusModeList)];
	int menuhandle[sizeof(statusModeList)];
}
	tStatusMenuHandles,*lpStatusMenuHandles;

lpStatusMenuHandles hStatusMenuHandles;
int hStatusMenuHandlesCnt;

extern HANDLE hStatusModeChangeEvent;
extern int GetProtocolVisibility(char * ProtoName);
extern int 	CheckProtocolOrder();

typedef struct{
	char *proto;
	int protoindex;
	int status;
}
	StatusMenuExecParam,*lpStatusMenuExecParam;

int StatusMenuExecService(WPARAM wParam,LPARAM lParam)
{
	lpStatusMenuExecParam smep=(lpStatusMenuExecParam)wParam;
	if (smep!=NULL) {
		PROTOCOLDESCRIPTOR **proto;
		int protoCount;
		int i;
		CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
		if ((smep->proto!=NULL))
		{
			NotifyEventHooks(hStatusModeChangeEvent, smep->status, (LPARAM)smep->proto);
			CallProtoService(smep->proto,PS_SETSTATUS,smep->status,0);
		}
		else {
			currentDesiredStatusMode=smep->status;
			NotifyEventHooks(hStatusModeChangeEvent,currentDesiredStatusMode,0);
			for(i=0;i<protoCount;i++)
				CallProtoService(proto[i]->szName,PS_SETSTATUS,currentDesiredStatusMode,0);

			DBWriteContactSettingWord(NULL,"CList","Status",(WORD)currentDesiredStatusMode);
			return 1;
		}
	}
	return(0);
}

int FreeOwnerDataStatusMenu (WPARAM wParam,LPARAM lParam)
{
	lpStatusMenuExecParam smep;
	smep=(lpStatusMenuExecParam)lParam;

	if (smep!=NULL) {
		FreeAndNil(&smep->proto);
		FreeAndNil(&smep);
	}
	return (0);
}

static int BuildStatusMenu(WPARAM wParam,LPARAM lParam)
{
	HMENU hMenu;
	ListParam param;

	memset(&param,0,sizeof(param));
	param.MenuObjectHandle=hStatusMenuObject;
	param.rootlevel=-1;

	hMenu=hStatusMenu;
	//clear statusmenu
	while (GetMenuItemCount(hStatusMenu)>0){DeleteMenu(hStatusMenu,0,MF_BYPOSITION);}

	CallService(MO_BUILDMENU,(WPARAM)hMenu,(LPARAM)&param);
	DrawMenuBar((HWND)CallService("CLUI/GetHwnd",(WPARAM)0,(LPARAM)0));
	return (int)hMenu;
}

int GetProtoIndexByPos(PROTOCOLDESCRIPTOR ** proto, int protoCnt, int Pos)
{
	int p;
	char buf[10];
	char * b2=NULL;
	_itoa(Pos,buf,10);
	b2=DBGetStringA(NULL,"Protocols",buf);

	if (b2) {
		for (p=0; p<protoCnt; p++)
			if ( lstrcmpA(proto[p]->szName,b2)==0) {
				mir_free(b2);
				return p;
			}
		mir_free(b2);
	}

	return -1;
}

int MenuModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	int i,j,protoCount=0,networkProtoCount,s;
	int storedProtoCount;
	PROTOCOLDESCRIPTOR **proto;
	DWORD statusFlags=0,flags;
	TMO_MenuItem tmi;
	TMenuParam tmp;
	int pos=0;
	OptParam op;

	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
	networkProtoCount=0;
	CheckProtocolOrder();

	//clear statusmenu
	while ( GetMenuItemCount(hStatusMenu) > 0 )
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
	//tmp.
	tmp.name="StatusMenu";

	hStatusMenuObject=(int)CallService(MO_CREATENEWMENUOBJECT,(WPARAM)0,(LPARAM)&tmp);
	{
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
	hStatusMainMenuHandlesCnt = SIZEOF(statusModeList);
	for (i=0;i<protoCount;i++)
		if (proto[i]->type==PROTOTYPE_PROTOCOL)
			networkProtoCount++;

	memset(hStatusMainMenuHandles,0,sizeof(statusModeList));
	hStatusMenuHandles=(tStatusMenuHandles*)mir_alloc(sizeof(tStatusMenuHandles)*protoCount);
	hStatusMenuHandlesCnt=protoCount;

	memset(hStatusMenuHandles,0,sizeof(tStatusMenuHandles)*protoCount);
	storedProtoCount=DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);

	for(s=0;s<storedProtoCount;s++) {
		pos=0;
		i=GetProtoIndexByPos(proto,protoCount,s);
		if (i==-1) continue;
		if ((proto[i]->type!=PROTOTYPE_PROTOCOL) || (DBGetContactSettingByte(NULL,"CLUI","DontHideStatusMenu",0)==0&&GetProtocolVisibility(proto[i]->szName)==0)) continue;

		flags=CallProtoService(proto[i]->szName,PS_GETCAPS,PFLAGNUM_2,0);
		if (networkProtoCount>1) {
			char protoName[128];
			int j;
			int rootmenu;
			//adding root
			memset(&tmi,0,sizeof(tmi));
			memset(protoName,0,128);
			tmi.cbSize=sizeof(tmi);
			tmi.flags=CMIF_CHILDPOPUP|CMIF_ROOTPOPUP;
			tmi.position=pos++;
			tmi.hIcon=(HICON)CallProtoService(proto[i]->szName,PS_LOADICON,PLI_PROTOCOL|PLIF_SMALL,0);
			tmi.root=-1;
			CallProtoService(proto[i]->szName,PS_GETNAME,SIZEOF(protoName),(LPARAM)protoName);
			tmi.pszName=protoName;
			rootmenu=CallService(MO_ADDNEWMENUITEM,(WPARAM)hStatusMenuObject,(LPARAM)&tmi);


					memset(&tmi,0,sizeof(tmi));
					tmi.cbSize=sizeof(tmi);
					tmi.flags=CMIF_CHILDPOPUP;
					//if (statusModeList[j]==ID_STATUS_OFFLINE){tmi.flags|=CMIF_CHECKED;}
					tmi.root=rootmenu;
					tmi.position=pos++;
					tmi.pszName=protoName;
					tmi.hIcon=(HICON)CallProtoService(proto[i]->szName,PS_LOADICON,PLI_PROTOCOL|PLIF_SMALL,0);
				op.Handle=CallService(MO_ADDNEWMENUITEM,(WPARAM)hStatusMenuObject,(LPARAM)&tmi);

			{
				char buf[256];
				sprintf(buf,"RootProtocolIcon_%s",proto[i]->szName);
				op.Value=(int)buf;
				op.Setting=OPT_MENUITEMSETUNIQNAME;
				CallService(MO_SETOPTIONSMENUITEM,(WPARAM)0,(LPARAM)&op);
			}

			pos+=100000;

			for(j=0; j < SIZEOF(statusModeList); j++) {
				if (!(flags&statusModePf2List[j])) {
					//	DeleteMenu(hMenu,statusModeList[j],MF_BYCOMMAND);
				}
				else {

					char *st;
					st=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,statusModeList[j],0);
					//adding
					memset(&tmi,0,sizeof(tmi));
					tmi.cbSize=sizeof(tmi);
					tmi.flags=CMIF_CHILDPOPUP;
					if (statusModeList[j]==ID_STATUS_OFFLINE)
						tmi.flags|=CMIF_CHECKED;
					tmi.root=rootmenu;
					tmi.position=pos++;
					tmi.pszName=st;
					tmi.hIcon=LoadSkinnedProtoIcon(proto[i]->szName,statusModeList[j]);
					{
						//owner data
						lpStatusMenuExecParam smep;
						smep=(lpStatusMenuExecParam)mir_alloc(sizeof(StatusMenuExecParam));
						smep->status=statusModeList[j];
						smep->protoindex=s;
						smep->proto=mir_strdup(proto[i]->szName);
						tmi.ownerdata=smep;
					}

					hStatusMenuHandles[i].protoindex=i;
					hStatusMenuHandles[i].protostatus[j]=statusModeList[j];
					hStatusMenuHandles[i].menuhandle[j]=CallService(MO_ADDNEWMENUITEM,(WPARAM)hStatusMenuObject,(LPARAM)&tmi);

					{
						char buf[256];
						op.Handle=hStatusMenuHandles[i].menuhandle[j];
						sprintf(buf,"ProtocolIcon_%s_%s",proto[i]->szName,tmi.pszName);
						op.Value=(int)buf;
						op.Setting=OPT_MENUITEMSETUNIQNAME;
						CallService(MO_SETOPTIONSMENUITEM,(WPARAM)0,(LPARAM)&op);
					}
				}
			}
		}
		statusFlags|=flags;
	}
	if (networkProtoCount>1)
		pos+=100000;

	{
		//add to root menu
		for(j=0;j<sizeof(statusModeList)/sizeof(statusModeList[0]);j++) {
			for(i=0;i<protoCount;i++) {
				if (proto[i]->type!=PROTOTYPE_PROTOCOL)//  || (DBGetContactSettingByte(NULL,"CLUI","DontHideStatusMenu",1) == 0)) // && GetProtocolVisibility(proto[i]->szName)==0))
					continue;
				flags=CallProtoService(proto[i]->szName,PS_GETCAPS,PFLAGNUM_2,0);
				if (flags&statusModePf2List[j]){
					//DeleteMenu(hMenu,statusModeList[j],MF_BYCOMMAND)
					int buf[256];
					memset(&tmi,0,sizeof(tmi));
					memset(&buf,0,256);
					tmi.cbSize=sizeof(tmi);
					tmi.flags=CMIF_CHILDPOPUP;
					if (statusModeList[j]==ID_STATUS_OFFLINE)
						tmi.flags|=CMIF_CHECKED;

					tmi.hIcon=LoadSkinnedIcon(skinIconStatusList[j]);
					tmi.position=pos++;
					tmi.root=-1;
					tmi.hotKey=MAKELPARAM(MOD_CONTROL,'0'+j);
					tmi.pszName=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,statusModeList[j],0);

					wsprintf((LPTSTR)buf, _T("%s\tCtrl-%c"), tmi.pszName,'0'+j);
					tmi.pszName=(char *)buf;
					{
						//owner data
						lpStatusMenuExecParam smep;
						smep=(lpStatusMenuExecParam)mir_alloc(sizeof(StatusMenuExecParam));
						smep->status=statusModeList[j];
						smep->proto=NULL;
						tmi.ownerdata=smep;
					}
					hStatusMainMenuHandles[j]=(void **)(CallService(MO_ADDNEWMENUITEM,(WPARAM)hStatusMenuObject,(LPARAM)&tmi));
					{
						char buf[256];
						op.Handle=(int)hStatusMainMenuHandles[j];
						sprintf(buf,"Root2ProtocolIcon_%s_%s",proto[i]->szName,tmi.pszName);
						op.Value=(int)buf;
						op.Setting=OPT_MENUITEMSETUNIQNAME;
						CallService(MO_SETOPTIONSMENUITEM,(WPARAM)0,(LPARAM)&op);
					}

					break;
				}
			}
		}
	}

	BuildStatusMenu(0,0);
	return 0;
}

int statustopos(int status)
{
	int j;
	if (statusModeList==NULL) return (-1);
	for(j=0; j < SIZEOF(statusModeList); j++)
		if (status==statusModeList[j])
			return(j);

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
	for(i=0;i<protoCount;i++)
		if (proto[i]->type==PROTOTYPE_PROTOCOL) {
			thisStatus=CallProtoService(proto[i]->szName,PS_GETSTATUS,0,0);
			if (overallStatus==0) overallStatus=thisStatus;
			else if (overallStatus!=thisStatus) overallStatus=-1;
			networkProtoCount++;
		}
		memset(&tmi,0,sizeof(tmi));
		tmi.cbSize=sizeof(tmi);
		if (overallStatus>ID_STATUS_CONNECTING) {
			int pos = statustopos(currentStatusMenuItem);
			if (pos>=0 && pos < hStatusMainMenuHandlesCnt) {
				tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
				CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMainMenuHandles[pos],(LPARAM)&tmi);
		}

		currentStatusMenuItem=overallStatus;
		
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

		currentStatusMenuItem=0;
	}
	if (networkProtoCount<=1) return 0;
	for(i=0;i<protoCount;i++)
		if (!strcmp(proto[i]->szName,ack->szModule)) break;

	//hProcess is previous mode, lParam is new mode
	if ((int)ack->hProcess>=ID_STATUS_OFFLINE && (int)ack->hProcess<ID_STATUS_OFFLINE + SIZEOF(statusModeList))
	{
		int pos = statustopos((int)ack->hProcess);
		if (pos>=0 && pos < sizeof(statusModeList)/sizeof(statusModeList[0])) {
			tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
			CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMenuHandles[i].menuhandle[pos],(LPARAM)&tmi);
	}	}

	if (ack->lParam>=ID_STATUS_OFFLINE && ack->lParam<ID_STATUS_OFFLINE+SIZEOF(statusModeList))
	{
		int pos = statustopos((int)ack->lParam);
		if (pos>=0 && pos < sizeof(statusModeList)/sizeof(statusModeList[0])) {
			tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|CMIF_CHECKED;
			CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMenuHandles[i].menuhandle[pos],(LPARAM)&tmi);
	}	}

	BuildStatusMenu(0,0);
	return 0;
}

int CloseAction(WPARAM wParam,LPARAM lParam)
{
	if (CallService(MS_SYSTEM_OKTOEXIT,(WPARAM)0,(LPARAM)0))
		SendMessage( pcli->hwndContactList,WM_DESTROY,0,0 );

	return(0);
}

int InitCustomMenus(void)
{
	CreateServiceFunction("StatusMenuExecService",StatusMenuExecService);

	CreateServiceFunction("CloseAction",CloseAction);

	//free services
	CreateServiceFunction("CLISTMENUS/FreeOwnerDataStatusMenu",FreeOwnerDataStatusMenu);

	hStatusMenu=GetSubMenu(LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(IDR_CLISTMENU)),1);
	CallService(MS_LANGPACK_TRANSLATEMENU,(WPARAM)hStatusMenu,0);

	hStatusMainMenuHandles=NULL;
	hStatusMainMenuHandlesCnt=0;

	hStatusMenuHandles=NULL;
	hStatusMenuHandlesCnt=0;

	currentStatusMenuItem=ID_STATUS_OFFLINE;
	currentDesiredStatusMode=ID_STATUS_OFFLINE;

	HookEvent(ME_SYSTEM_MODULESLOADED,MenuModulesLoaded);
	return 0;
}

void UninitCustomMenus(void)
{
	if (hStatusMainMenuHandles!=NULL)
		mir_free(hStatusMainMenuHandles);
	hStatusMainMenuHandles=NULL;

	if ( hStatusMenuHandles != NULL )
		mir_free(hStatusMenuHandles);
	hStatusMenuHandles=NULL;
}
