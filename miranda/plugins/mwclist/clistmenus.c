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
#include "clist.h"
#include "genmenu.h"
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
static int hMainMenuObject;
static int hContactMenuObject;
static int hStatusMenuObject;
int MenuModulesLoaded(WPARAM,LPARAM);

int statustopos(int status);
//
//HIMAGELIST hImlMenuIcons;

//static HANDLE hPreBuildContactMenuEvent;
static HANDLE hPreBuildMainMenuEvent;

static HANDLE hPreBuildContactMenuEvent,hAckHook;

static HMENU hMainMenu,hStatusMenu;
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
//mainmenu exec param(ownerdata)
typedef struct{
char *szServiceName;
int Param1;
}MainMenuExecParam,*lpMainMenuExecParam;
//


//contactmenu exec param(ownerdata)
//also used in checkservice
typedef struct{
char *szServiceName;
char *pszContactOwner;//for check proc
int param;
}ContactMenuExecParam,*lpContactMenuExecParam;

typedef struct {
	char *szProto;
	int isOnList;
	int isOnline;
} BuildContactParam;


typedef struct{
char *proto;
int protoindex;
int status;
}StatusMenuExecParam,*lpStatusMenuExecParam;

//////////////////////////////MAIN MENU/////////////////////////

/*
wparam=handle to the menu item returned by MS_CLIST_ADDCONTACTMENUITEM
return 0 on success.
*/
static int RemoveMainMenuItem(WPARAM wParam,LPARAM lParam)
{
/*not need if used free service
	lpMainMenuExecParam mmep;

	mmep=CallService(MO_MENUITEMGETOWNERDATA,wParam,lParam);
	
	if (mmep!=NULL){
		FreeAndNil(&mmep->szServiceName);
		mir_free(mmep);
	}
*/
	CallService(MO_REMOVEMENUITEM,wParam,0);
	return 0;
}


static int BuildMainMenu(WPARAM wParam,LPARAM lParam)
{
	int tick;
	HMENU hMenu;
	ListParam param;

	memset(&param,0,sizeof(param));
	param.MenuObjectHandle=hMainMenuObject;
	param.rootlevel=-1;

	hMenu=hMainMenu;
	tick=GetTickCount();
	NotifyEventHooks(hPreBuildMainMenuEvent,(WPARAM)0,(LPARAM)0);

	CallService(MO_BUILDMENU,(WPARAM)hMenu,(LPARAM)&param);
	DrawMenuBar((HWND)CallService("CLUI/GetHwnd",(WPARAM)0,(LPARAM)0));
	tick=GetTickCount()-tick;
	return (int)hMenu;
}

static int AddMainMenuItem(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM *mi=(CLISTMENUITEM*)lParam;
	TMO_MenuItem tmi;
	OptParam op;

	if(mi->cbSize!=sizeof(CLISTMENUITEM)) return 0;

	memset(&tmi,0,sizeof(tmi));
	tmi.cbSize=sizeof(tmi);
	tmi.flags=mi->flags;
	tmi.hIcon=mi->hIcon;
	tmi.hotKey=mi->hotKey;
	tmi.pszName=mi->pszName;
	tmi.position=mi->position;

	//pszPopupName for new system mean root level
        //pszPopupName for old system mean that exists popup
	tmi.root=(int)mi->pszPopupName;

	tmi.ownerdata=NULL;
	
	{
		lpMainMenuExecParam mmep;
		mmep=(lpMainMenuExecParam)mir_alloc(sizeof(MainMenuExecParam));
		if(mmep==NULL){return(0);}
		
		//we need just one parametr.
		mmep->szServiceName=mir_strdup(mi->pszService);
		mmep->Param1=mi->popupPosition;
		
		tmi.ownerdata=mmep;
	}
	op.Handle=CallService(MO_ADDNEWMENUITEM,(WPARAM)hMainMenuObject,(LPARAM)&tmi);
	op.Setting=OPT_MENUITEMSETUNIQNAME;
	op.Value=(int)mi->pszService;
	CallService(MO_SETOPTIONSMENUITEM,(WPARAM)0,(LPARAM)&op);
	return(op.Handle);

//	mainItemCount++;
//	return MENU_CUSTOMITEMMAIN|(mainMenuItem[mainItemCount-1].id);
}

int MainMenuCheckService(WPARAM wParam,LPARAM lParam) {
//not used
	return(0);
}

//called with:
//wparam - ownerdata
//lparam - lparam from winproc
int MainMenuExecService(WPARAM wParam,LPARAM lParam) {
	if (wParam!=0)
	{
		lpMainMenuExecParam mmep=(lpMainMenuExecParam)wParam;
		if (!strcmp(mmep->szServiceName,"Help/AboutCommand"))
		{
			//bug in help.c,it used wparam as parent window handle without reason.
			mmep->Param1=0;
		}
		CallService(mmep->szServiceName,mmep->Param1,lParam);
		

	}
	return(1);
}
int FreeOwnerDataMainMenu (WPARAM wParam,LPARAM lParam)
{
	lpMainMenuExecParam mmep;
	mmep=(lpMainMenuExecParam)lParam;
	if (mmep!=NULL){
		FreeAndNil(&mmep->szServiceName);
		FreeAndNil(&mmep);
	}
return(0);
}



//////////////////////////////END MAIN MENU/////////////////////////



//////////////////////////////CONTACT MENU/////////////////////////

static int RemoveContactMenuItem(WPARAM wParam,LPARAM lParam)
{
	//identical

	CallService(MO_REMOVEMENUITEM,wParam,0);
	return 0;
}

static int AddContactMenuItem(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM *mi=(CLISTMENUITEM*)lParam;
	TMO_MenuItem tmi;
	OptParam op;

	if(mi->cbSize!=sizeof(CLISTMENUITEM)) return 0;

	memset(&tmi,0,sizeof(tmi));
	tmi.cbSize=sizeof(tmi);
	tmi.flags=mi->flags;
	tmi.hIcon=mi->hIcon;
	tmi.hotKey=mi->hotKey;
	tmi.position=mi->position;
	tmi.pszName=mi->pszName;
	tmi.root=(int)mi->pszPopupName;

	if(!(mi->flags&CMIF_ROOTPOPUP||mi->flags&CMIF_CHILDPOPUP)) {
		//old system
		tmi.flags|=CMIF_CHILDPOPUP;
		tmi.root=-1;
	}


	{
		//owner data
		lpContactMenuExecParam cmep;
		cmep=(lpContactMenuExecParam)mir_alloc(sizeof(ContactMenuExecParam));
		memset(cmep,0,sizeof(ContactMenuExecParam));
		cmep->szServiceName=mir_strdup(mi->pszService);
		if (mi->pszContactOwner!=NULL) cmep->pszContactOwner=mir_strdup(mi->pszContactOwner);
		cmep->param=mi->popupPosition;	
		tmi.ownerdata=cmep;
	}
	
	op.Handle=(int)CallService(MO_ADDNEWMENUITEM,(WPARAM)hContactMenuObject,(LPARAM)&tmi);	
	op.Setting=OPT_MENUITEMSETUNIQNAME;
	{
			char buf[256];
			wsprintfA(buf,"%s/%s",mi->pszContactOwner?mi->pszContactOwner:"",mi->pszService?mi->pszService:"");
			op.Value=(int)buf;
	CallService(MO_SETOPTIONSMENUITEM,(WPARAM)0,(LPARAM)&op);
	}
	return(op.Handle);

	

}
static int BuildContactMenu(WPARAM wParam,LPARAM lParam)
{
	HMENU hMenu;
	int isOnline,isOnList;
	HANDLE hContact=(HANDLE)wParam;
	char *szProto;
	BuildContactParam bcp;
	ListParam param;

	NotifyEventHooks(hPreBuildContactMenuEvent,(WPARAM)hContact,0);
	
	szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(WPARAM)hContact,0);
	isOnList=0==DBGetContactSettingByte(hContact,"CList","NotOnList",0);
	isOnline=szProto!=NULL && ID_STATUS_OFFLINE!=DBGetContactSettingWord(hContact,szProto,"Status",ID_STATUS_OFFLINE);
	
	bcp.szProto=szProto;
	bcp.isOnList=isOnList;
	bcp.isOnline=isOnline;

	memset(&param,0,sizeof(param));

	param.MenuObjectHandle=hContactMenuObject;
	param.rootlevel=-1;
	param.wParam=(WPARAM)&bcp;

	hMenu=CreatePopupMenu();
	CallService(MO_BUILDMENU,(WPARAM)hMenu,(LPARAM)&param);

	return (int)hMenu;
}

//called with:
//wparam - ownerdata
//lparam - lparam from winproc
int ContactMenuExecService(WPARAM wParam,LPARAM lParam) {
	if (wParam!=0)
	{
		lpContactMenuExecParam cmep=(lpContactMenuExecParam)wParam;
	//call with wParam=(WPARAM)(HANDLE)hContact,lparam=popupposition
		CallService(cmep->szServiceName,lParam,cmep->param);
		
	}
	return(0);
}

//true - ok,false ignore
int ContactMenuCheckService(WPARAM wParam,LPARAM lParam) {
	
	PCheckProcParam pcpp=(PCheckProcParam)wParam;
	BuildContactParam *bcp=NULL;
	lpContactMenuExecParam cmep=NULL;
	TMO_MenuItem mi;
	
	if (pcpp==NULL){return(FALSE);}
	bcp=(BuildContactParam *)pcpp->wParam;
	if (bcp==NULL){return(FALSE);}

	cmep=pcpp->MenuItemOwnerData;
	if (cmep==NULL){return(TRUE);}//this is root...build it

	if(cmep->pszContactOwner!=NULL) {
		if(bcp->szProto==NULL) return(FALSE);
		if(strcmp(cmep->pszContactOwner,bcp->szProto)) return(FALSE);
	}
	if (MO_GetMenuItem((WPARAM)pcpp->MenuItemHandle,(LPARAM)&mi)==0)
	{
	if(mi.flags&CMIF_HIDDEN) return(FALSE);
	if(mi.flags&CMIF_NOTONLIST && bcp->isOnList) return(FALSE);
	if(mi.flags&CMIF_NOTOFFLIST && !(bcp->isOnList)) return(FALSE);
	if(mi.flags&CMIF_NOTONLINE && (bcp->isOnline)) return(FALSE);
	if(mi.flags&CMIF_NOTOFFLINE && !(bcp->isOnline)) return(FALSE);
	}
	return TRUE;

	return(0);
}

int FreeOwnerDataContactMenu (WPARAM wParam,LPARAM lParam)
{

	lpContactMenuExecParam cmep;

	cmep=(lpContactMenuExecParam)lParam;
	if (cmep!=NULL){
		FreeAndNil(&cmep->szServiceName);
		FreeAndNil(&cmep->pszContactOwner);
		FreeAndNil(&cmep);
	}
	return(0);
}


//////////////////////////////END CONTACT MENU/////////////////////////


//wparam MenuItemHandle
static int ModifyCustomMenuItem(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM *mi=(CLISTMENUITEM*)lParam;
	TMO_MenuItem tmi;
	
	if (lParam==0){return(-1);}
	if(mi->cbSize!=sizeof(CLISTMENUITEM)) return 1;
	
	
	tmi.cbSize=sizeof(tmi);
	tmi.flags=mi->flags;
	tmi.hIcon=mi->hIcon;
	tmi.hotKey=mi->hotKey;
	tmi.pszName=mi->pszName;
	//todo new flags for this parametrs
	//tmi.ownerdata
	//tmi.position
	return((int)CallService(MO_MODIFYMENUITEM,wParam,(LPARAM)&tmi));
}

int MenuProcessCommand(WPARAM wParam,LPARAM lParam)
{
	if ((HIWORD(wParam)&MPCF_CONTACTMENU)||(HIWORD(wParam)&MPCF_MAINMENU))
	{
		//process old menu sys
		if (HIWORD(wParam)&MPCF_CONTACTMENU)
		{
			//make faked globalid
			return(CallService(MO_PROCESSCOMMAND,getGlobalId(hContactMenuObject,LOWORD(wParam)),lParam));
		}
		if (HIWORD(wParam)&MPCF_MAINMENU)
		{
			int hst=LOWORD(wParam);
			if ((hst>=ID_STATUS_OFFLINE)&&(hst<=ID_STATUS_OUTTOLUNCH))
			{
				int pos=statustopos(hst);
				if (pos!=-1&&hStatusMainMenuHandles!=NULL)
				{
					OutputDebugStringA("Processing WM_COMAND for status menus\r\n");
					return(CallService(MO_PROCESSCOMMAND,(WPARAM)hStatusMainMenuHandles[pos],lParam));
				}
				
			}
			//make faked globalid
			
		}
		
		//unknown old menu
		return(CallService(MO_PROCESSCOMMANDBYMENUIDENT,LOWORD(wParam),lParam));	
	}
	else
	{
	
	/*
	if (!IsBadCodePtr(wParam))
	{
		pcp=(lpProcessCommandParam)wParam;
		mii.cbSize=sizeof(mii);
		mii.fMask=MIIM_DATA;
		
		if (GetMenuItemInfo(pcp->menu,pcp->ident,FALSE,&mii)!=0)
		{
			return(CallService(MO_PROCESSCOMMAND,mii.dwItemData,pcp->lParam));
		}
	}
	*/
	//prefered to call top way but only if caller KNOW hmenu 
	return(CallService(MO_PROCESSCOMMANDBYMENUIDENT,LOWORD(wParam),lParam));	

	}
	
	return 0;
}
int StatusMenuExecService(WPARAM wParam,LPARAM lParam)
{
	lpStatusMenuExecParam smep=(lpStatusMenuExecParam)wParam;
	if (smep!=NULL)
	{
			PROTOCOLDESCRIPTOR **proto;
			int protoCount;
			int i;
			CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);

		if ((smep->proto!=NULL))
		{
			NotifyEventHooks(hStatusModeChangeEvent, smep->status, (LPARAM)smep->proto);
			CallProtoService(smep->proto,PS_SETSTATUS,smep->status,0);	
		}else
		{
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

	if (smep!=NULL)
	{
		FreeAndNil(&smep->proto);
		FreeAndNil(&smep);
	}

return (0);
}

static int MenuProcessHotkey(WPARAM vKey,LPARAM lParam)
{
	int res;
/*
	if(lParam&MPCF_MAINMENU) {
		if(vKey>='0' && vKey<='9' && GetKeyState(VK_CONTROL)&0x8000 && !(GetKeyState(VK_MENU)&0x8000) && !(GetKeyState(VK_SHIFT)&0x8000)) {
			MenuProcessCommand(MAKEWPARAM(statusModeList[vKey-'0'],MPCF_MAINMENU),0);
			return 1;
		}
	}
*/
	res=CallService(MO_PROCESSHOTKEYS,(WPARAM)hStatusMenuObject,(LPARAM)vKey);
	if (res){return(res);}
	res=CallService(MO_PROCESSHOTKEYS,(WPARAM)hMainMenuObject,(LPARAM)vKey);
	if (res){return(res);}
	return 0;
}

static int MenuIconsChanged(WPARAM wParam,LPARAM lParam)
{

	//just rebuild menu
	MenuModulesLoaded(0,0);

	return 0;
}

static void GiveExistingItemAnIcon(UINT id,HICON hIcon)
{
}

static int MeasureMenuItem(WPARAM wParam,LPARAM lParam)
{
//	LPMEASUREITEMSTRUCT mis=(LPMEASUREITEMSTRUCT)lParam;
	return(CallService(MO_MEASUREMENUITEM,(WPARAM)0,lParam));
}

static int DrawMenuItem(WPARAM wParam,LPARAM lParam)
{
	//LPDRAWITEMSTRUCT dis=(LPDRAWITEMSTRUCT)lParam;
	return (CallService(MO_DRAWMENUITEM,(WPARAM)0,lParam));
}

static int MenuGetMain(WPARAM wParam,LPARAM lParam)
{
	if (hMainMenu!=NULL){
		while(GetMenuItemCount(hMainMenu)>0){DeleteMenu(hMainMenu,0,MF_BYPOSITION);}
	}
	BuildMainMenu(0,0);
	return (int)hMainMenu;
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
	while (GetMenuItemCount(hStatusMenu)>0){DeleteMenu(hStatusMenu,0,MF_BYPOSITION);}

	tick=GetTickCount();
	//NotifyEventHooks(hPreBuildMainMenuEvent,0,0);

	CallService(MO_BUILDMENU,(WPARAM)hMenu,(LPARAM)&param);
	DrawMenuBar((HWND)CallService("CLUI/GetHwnd",(WPARAM)0,(LPARAM)0));
	tick=GetTickCount()-tick;
	return (int)hMenu;
}


static int MenuGetStatus(WPARAM wParam,LPARAM lParam)
{
	//return (int)hStatusMenu;
	return BuildStatusMenu(0,0);
}


int freeownerdataformenus()
{
/* not needed due free service
	void *data;
	int i;
	if (hStatusMainMenuHandles!=NULL)
	{
	for (i=0;i<hStatusMainMenuHandlesCnt;i++)
	{


		data=(void *)CallService(MO_MENUITEMGETOWNERDATA,hStatusMainMenuHandles[i],0);
		if (data!=NULL)
		{
			mir_free(data);
			data=NULL;
		}

	}
	}

	if (hStatusMenuHandles!=NULL)
	{

	for (i=0;i<hStatusMenuHandlesCnt;i++)
	{
		data=(void *)CallService(MO_MENUITEMGETOWNERDATA,hStatusMenuHandles[i].menuhandle,0);
		if (data!=NULL)
		{
			mir_free(data);
			data=NULL;
		}

	}
	}
	*/
	return(1);
}

int GetProtoIndexByPos(PROTOCOLDESCRIPTOR ** proto, int protoCnt, int Pos)
{
	int res=0;
	int p;
	char buf[10];
	char * b2=NULL;
	/*
	spcnt=DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);
	itoa(Pos+200,buf,10);
	p=DBGetContactSettingDword(0,"Protocols",buf,-1);
	*/
	itoa(Pos,buf,10);
	b2=DBGetStringA(NULL,"Protocols",buf);
	OutputDebugStringA("GetProtoIndexByPos \r\n");
	
	if (b2) {
		for (p=0; p<protoCnt; p++)
			if (strcmp(proto[p]->szName,b2)==0) {
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
	
	//
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
	networkProtoCount=0;
	CheckProtocolOrder();

	//
	
	//clear statusmenu
	while (GetMenuItemCount(hStatusMenu)>0){DeleteMenu(hStatusMenu,0,MF_BYPOSITION);}
	
	//status menu
	if (hStatusMenuObject!=0)
	{
		freeownerdataformenus();
		CallService(MO_REMOVEMENUOBJECT,hStatusMenuObject,0);
		if (hStatusMainMenuHandles!=NULL){mir_free(hStatusMainMenuHandles);}
		if (hStatusMenuHandles!=NULL){mir_free(hStatusMenuHandles);}
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
	for(i=0;i<protoCount;i++)
	{
		if(proto[i]->type==PROTOTYPE_PROTOCOL && (DBGetContactSettingByte(NULL,"CLUI","DontHideStatusMenu",0)||GetProtocolVisibility(proto[i]->szName)!=0)) networkProtoCount++;
	}

	memset(hStatusMainMenuHandles,0,sizeof(statusModeList));
	hStatusMenuHandles=(tStatusMenuHandles*)mir_alloc(sizeof(tStatusMenuHandles)*protoCount);
	hStatusMenuHandlesCnt=protoCount;

	memset(hStatusMenuHandles,0,sizeof(tStatusMenuHandles)*protoCount);
	storedProtoCount=DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);
	
	for(s=0;s<storedProtoCount;s++) {
		pos=0;
		i=GetProtoIndexByPos(proto,protoCount,s);
		if (i==-1) continue;
		if((proto[i]->type!=PROTOTYPE_PROTOCOL) || (DBGetContactSettingByte(NULL,"CLUI","DontHideStatusMenu",0)==0&&GetProtocolVisibility(proto[i]->szName)==0)) continue;
		
		flags=CallProtoService(proto[i]->szName,PS_GETCAPS,PFLAGNUM_2,0);
		if(networkProtoCount>1) {
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
					//if(statusModeList[j]==ID_STATUS_OFFLINE){tmi.flags|=CMIF_CHECKED;}
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
				if(!(flags&statusModePf2List[j])) 
				{
				//	DeleteMenu(hMenu,statusModeList[j],MF_BYCOMMAND);
				}
				else {
					
					char *st;
					st=(char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,statusModeList[j],0);
					//adding
					memset(&tmi,0,sizeof(tmi));
					tmi.cbSize=sizeof(tmi);
					tmi.flags=CMIF_CHILDPOPUP;
					if(statusModeList[j]==ID_STATUS_OFFLINE){tmi.flags|=CMIF_CHECKED;}
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
		if(networkProtoCount>1) {
		pos+=100000;
	}

	{
		//add to root menu
		for(j=0;j<sizeof(statusModeList)/sizeof(statusModeList[0]);j++) {
			for(i=0;i<protoCount;i++) {
				if(proto[i]->type!=PROTOTYPE_PROTOCOL || (DBGetContactSettingByte(NULL,"CLUI","DontHideStatusMenu",0)==0&&GetProtocolVisibility(proto[i]->szName)==0)) continue;
				flags=CallProtoService(proto[i]->szName,PS_GETCAPS,PFLAGNUM_2,0);
				if(flags&statusModePf2List[j]){
					//DeleteMenu(hMenu,statusModeList[j],MF_BYCOMMAND)
					int buf[256];
					memset(&tmi,0,sizeof(tmi));
					memset(&buf,0,256);
					tmi.cbSize=sizeof(tmi);
					tmi.flags=CMIF_CHILDPOPUP;
					if(statusModeList[j]==ID_STATUS_OFFLINE){tmi.flags|=CMIF_CHECKED;}
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

	if(ack->type!=ACKTYPE_STATUS) return 0;
	if(ack->result!=ACKRESULT_SUCCESS) return 0;
	if (hStatusMainMenuHandles==NULL) return(0);
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
	networkProtoCount=0;
	for(i=0;i<protoCount;i++)
		if(proto[i]->type==PROTOTYPE_PROTOCOL) {
			thisStatus=CallProtoService(proto[i]->szName,PS_GETSTATUS,0,0);
			if(overallStatus==0) overallStatus=thisStatus;
			else if(overallStatus!=thisStatus) overallStatus=-1;
			networkProtoCount++;
		}
	memset(&tmi,0,sizeof(tmi));
	tmi.cbSize=sizeof(tmi);
	if(overallStatus>ID_STATUS_CONNECTING) {
		//tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
		//CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMainMenuHandles[statustopos(currentStatusMenuItem)],(LPARAM)&tmi);
		int pos = statustopos(currentStatusMenuItem);
		if (pos>=0 && pos < hStatusMainMenuHandlesCnt) {
			tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
			CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMainMenuHandles[pos],(LPARAM)&tmi);
		}		
		
		currentStatusMenuItem=overallStatus;		
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

		currentStatusMenuItem=0;
	}
	if(networkProtoCount<=1) return 0;
	for(i=0;i<protoCount;i++)
		if(!strcmp(proto[i]->szName,ack->szModule)) break;

	//hProcess is previous mode, lParam is new mode
	if((int)ack->hProcess>=ID_STATUS_OFFLINE && (int)ack->hProcess<ID_STATUS_OFFLINE + SIZEOF(statusModeList))
	{
		int pos = statustopos((int)ack->hProcess);
		if (pos>=0 && pos < sizeof(statusModeList)/sizeof(statusModeList[0])) {
			tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP;
			CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMenuHandles[i].menuhandle[pos],(LPARAM)&tmi);
	}	}	

	if(ack->lParam>=ID_STATUS_OFFLINE && ack->lParam<ID_STATUS_OFFLINE+SIZEOF(statusModeList))
	{
		int pos = statustopos((int)ack->lParam);
		if (pos>=0 && pos < sizeof(statusModeList)/sizeof(statusModeList[0])) {
			tmi.flags=CMIM_FLAGS|CMIF_CHILDPOPUP|CMIF_CHECKED;
			CallService(MO_MODIFYMENUITEM,(WPARAM)hStatusMenuHandles[i].menuhandle[pos],(LPARAM)&tmi);
	}	}

	BuildStatusMenu(0,0);
	return 0;
}












static int MenuModulesShutdown(WPARAM wParam,LPARAM lParam) {
	UnhookEvent(hAckHook);
	return 0;
}

int CloseAction(WPARAM wParam,LPARAM lParam)
{
	if(CallService(MS_SYSTEM_OKTOEXIT,(WPARAM)0,(LPARAM)0))
		SendMessage((HWND)CallService(MS_CLUI_GETHWND,(WPARAM)0,(LPARAM)0),WM_DESTROY,0,0);

	return(0);
}

int InitCustomMenus(void)
{
	TMenuParam tmp;
	OptParam op;

	CreateServiceFunction("MainMenuExecService",MainMenuExecService);
	
	CreateServiceFunction("ContactMenuExecService",ContactMenuExecService);
	CreateServiceFunction("ContactMenuCheckService",ContactMenuCheckService);

	CreateServiceFunction("StatusMenuExecService",StatusMenuExecService);

	CreateServiceFunction("CloseAction",CloseAction);

//free services
	CreateServiceFunction("CLISTMENUS/FreeOwnerDataMainMenu",FreeOwnerDataMainMenu);
	CreateServiceFunction("CLISTMENUS/FreeOwnerDataContactMenu",FreeOwnerDataContactMenu);
	CreateServiceFunction("CLISTMENUS/FreeOwnerDataStatusMenu",FreeOwnerDataStatusMenu);

	CreateServiceFunction(MS_CLIST_ADDMAINMENUITEM,AddMainMenuItem);
	CreateServiceFunction(MS_CLIST_MENUGETMAIN,MenuGetMain);
	CreateServiceFunction(MS_CLIST_REMOVEMAINMENUITEM,RemoveMainMenuItem);
	CreateServiceFunction(MS_CLIST_MENUBUILDMAIN,BuildMainMenu);

	CreateServiceFunction(MS_CLIST_ADDCONTACTMENUITEM,AddContactMenuItem);
	CreateServiceFunction(MS_CLIST_MENUBUILDCONTACT,BuildContactMenu);
	CreateServiceFunction(MS_CLIST_REMOVECONTACTMENUITEM,RemoveContactMenuItem);
	

	CreateServiceFunction(MS_CLIST_MODIFYMENUITEM,ModifyCustomMenuItem);
	CreateServiceFunction(MS_CLIST_MENUMEASUREITEM,MeasureMenuItem);
	CreateServiceFunction(MS_CLIST_MENUDRAWITEM,DrawMenuItem);

	
	CreateServiceFunction(MS_CLIST_MENUGETSTATUS,MenuGetStatus);
	CreateServiceFunction(MS_CLIST_MENUPROCESSCOMMAND,MenuProcessCommand);
	CreateServiceFunction(MS_CLIST_MENUPROCESSHOTKEY,MenuProcessHotkey);
	
	hPreBuildContactMenuEvent=CreateHookableEvent(ME_CLIST_PREBUILDCONTACTMENU);
	hPreBuildMainMenuEvent=CreateHookableEvent(ME_CLIST_PREBUILDMAINMENU);

	hAckHook=(HANDLE)HookEvent(ME_PROTO_ACK,MenuProtoAck);

	hMainMenu=GetSubMenu(LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(IDR_CLISTMENU)),0);
	hStatusMenu=GetSubMenu(LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(IDR_CLISTMENU)),1);
	CallService(MS_LANGPACK_TRANSLATEMENU,(WPARAM)hMainMenu,0);
	CallService(MS_LANGPACK_TRANSLATEMENU,(WPARAM)hStatusMenu,0);

	hStatusMainMenuHandles=NULL;
	hStatusMainMenuHandlesCnt=0;

	hStatusMenuHandles=NULL;
	hStatusMenuHandlesCnt=0;

	//new menu sys
	InitGenMenu();

	//main menu
	memset(&tmp,0,sizeof(tmp));
	tmp.cbSize=sizeof(tmp);
	tmp.CheckService=NULL;
	tmp.ExecService="MainMenuExecService";
	tmp.name="Main Menu";
	hMainMenuObject=CallService(MO_CREATENEWMENUOBJECT,(WPARAM)0,(LPARAM)&tmp);
	
	
	op.Handle=hMainMenuObject;
	op.Setting=OPT_USERDEFINEDITEMS;
	op.Value=(int)TRUE;
	CallService(MO_SETOPTIONSMENUOBJECT,(WPARAM)0,(LPARAM)&op);
	
	op.Handle=hMainMenuObject;
	op.Setting=OPT_MENUOBJECT_SET_FREE_SERVICE;
	op.Value=(int)"CLISTMENUS/FreeOwnerDataMainMenu";
	CallService(MO_SETOPTIONSMENUOBJECT,(WPARAM)0,(LPARAM)&op);


	//contact menu
	memset(&tmp,0,sizeof(tmp));
	tmp.cbSize=sizeof(tmp);
	tmp.CheckService="ContactMenuCheckService";
	tmp.ExecService="ContactMenuExecService";
	tmp.name="Contact Menu";
	hContactMenuObject=CallService(MO_CREATENEWMENUOBJECT,(WPARAM)0,(LPARAM)&tmp);
	
	op.Handle=hContactMenuObject;
	op.Setting=OPT_USERDEFINEDITEMS;
	op.Value=(int)TRUE;
	CallService(MO_SETOPTIONSMENUOBJECT,(WPARAM)0,(LPARAM)&op);

	op.Handle=hContactMenuObject;
	op.Setting=OPT_MENUOBJECT_SET_FREE_SERVICE;
	op.Value=(int)"CLISTMENUS/FreeOwnerDataContactMenu";
	CallService(MO_SETOPTIONSMENUOBJECT,(WPARAM)0,(LPARAM)&op);

	{	//add  exit command to menu
		CLISTMENUITEM mi;
		memset(&mi,0,sizeof(mi));
		mi.cbSize=sizeof(mi);
		mi.position=0x7fffffff;
		mi.pszService="CloseAction";
		mi.pszName=Translate("E&xit");
		AddMainMenuItem((WPARAM)0,(LPARAM)&mi);
	}
	
	currentStatusMenuItem=ID_STATUS_OFFLINE;
	currentDesiredStatusMode=ID_STATUS_OFFLINE;

	if(IsWinVer98Plus()) {
		HookEvent(ME_SKIN_ICONSCHANGED,MenuIconsChanged);
	}
	HookEvent(ME_SYSTEM_MODULESLOADED,MenuModulesLoaded);
	HookEvent(ME_SYSTEM_SHUTDOWN,MenuModulesShutdown);
	return 0;
}

void UninitCustomMenus(void)
{
	if (hStatusMainMenuHandles!=NULL){mir_free(hStatusMainMenuHandles);}
	hStatusMainMenuHandles=NULL;

	if (hStatusMenuHandles!=NULL){mir_free(hStatusMenuHandles);}
	hStatusMenuHandles=NULL;

	UnitGenMenu();
}