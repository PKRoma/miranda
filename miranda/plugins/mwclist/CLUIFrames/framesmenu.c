#include "..\commonheaders.h"

//==========================Frames
int hFrameMenuObject;
static HANDLE hPreBuildFrameMenuEvent;
extern int InitCustomMenus(void);

//contactmenu exec param(ownerdata)
//also used in checkservice
typedef struct{
char *szServiceName;
int Frameid;
int param1;
}FrameMenuExecParam,*lpFrameMenuExecParam;


static int AddContextFrameMenuItem(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM *mi=(CLISTMENUITEM*)lParam;
	TMO_MenuItem tmi;
	
	if(mi->cbSize!=sizeof(CLISTMENUITEM)) return 0;

	memset(&tmi,0,sizeof(tmi));
	
	tmi.cbSize=sizeof(tmi);
	tmi.flags=mi->flags;
	tmi.hIcon=mi->hIcon;
	tmi.hotKey=mi->hotKey;
	tmi.position=mi->position;
	tmi.pszName=mi->pszName;
	
	if(mi->flags&CMIF_ROOTPOPUP||mi->flags&CMIF_CHILDPOPUP)
		tmi.root=(int)mi->pszPopupName;
	{
		lpFrameMenuExecParam fmep;
		fmep=(lpFrameMenuExecParam)malloc(sizeof(FrameMenuExecParam));
		if (fmep==NULL){return(0);};
		fmep->szServiceName=_strdup(mi->pszService);
		fmep->Frameid=mi->popupPosition;
		fmep->param1=(int)mi->pszContactOwner;
		
		tmi.ownerdata=fmep;
	};
	
	return(CallService(MO_ADDNEWMENUITEM,(WPARAM)hFrameMenuObject,(LPARAM)&tmi));
}

static int RemoveContextFrameMenuItem(WPARAM wParam,LPARAM lParam)
{
	lpFrameMenuExecParam fmep;
	fmep=(lpFrameMenuExecParam)CallService(MO_MENUITEMGETOWNERDATA,wParam,lParam);
	if (fmep!=NULL){
		if (fmep->szServiceName!=NULL){
			free(fmep->szServiceName);
			fmep->szServiceName=NULL;
		};
		free(fmep);
	}

	CallService(MO_REMOVEMENUITEM,wParam,0);
	return 0;
}

//called with:
//wparam - ownerdata
//lparam - lparam from winproc
int FrameMenuExecService(WPARAM wParam,LPARAM lParam) {
	lpFrameMenuExecParam fmep=(lpFrameMenuExecParam)wParam;
	if (fmep==NULL){return(-1);};
		CallService(fmep->szServiceName,lParam,fmep->param1);	

		return(0);
};

//true - ok,false ignore
int FrameMenuCheckService(WPARAM wParam,LPARAM lParam) {
	
	PCheckProcParam pcpp=(PCheckProcParam)wParam;
	lpFrameMenuExecParam fmep;
	TMO_MenuItem mi;

	if (pcpp==NULL){return(FALSE);};
	if (CallService(MO_GETMENUITEM,(WPARAM)pcpp->MenuItemHandle,(LPARAM)&mi)==0)
	{
		fmep=mi.ownerdata;
		if (fmep!=NULL)
		{
		//pcpp->wParam  -  frameid
		if (((WPARAM)fmep->Frameid==pcpp->wParam)||fmep->Frameid==-1) return(TRUE);	
		};

	};
	return(FALSE);
};

static int ContextFrameMenuNotify(WPARAM wParam,LPARAM lParam)
{
	NotifyEventHooks(hPreBuildFrameMenuEvent,wParam,lParam);
	return(0);
};

static int BuildContextFrameMenu(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM *mi=(CLISTMENUITEM*)lParam;
	HMENU hMenu;
	ListParam param;

	memset(&param,0,sizeof(param));

	param.MenuObjectHandle=hFrameMenuObject;
	param.wParam=wParam;
	param.lParam=lParam;
	param.rootlevel=-1;
	
	hMenu=CreatePopupMenu();
	//NotifyEventHooks(hPreBuildFrameMenuEvent,wParam,-1);
	ContextFrameMenuNotify(wParam,-1);
	CallService(MO_BUILDMENU,(WPARAM)hMenu,(LPARAM)&param);
	return (int)hMenu;
}

//==========================Frames end
boolean InternalGenMenuModule=FALSE;

int MeasureItemProxy(WPARAM wParam,LPARAM lParam) {

int val;
	if (InternalGenMenuModule) 
{

	val=CallService(MS_INT_MENUMEASUREITEM,wParam,lParam);
	if (val) return(val);
};
return CallService(MS_CLIST_MENUMEASUREITEM,wParam,lParam);


};


int DrawItemProxy(WPARAM wParam,LPARAM lParam) {
if (InternalGenMenuModule) 
{
int val;
	val=CallService(MS_INT_MENUDRAWITEM,wParam,lParam);
	if (val) return(val);
}
return CallService(MS_CLIST_MENUDRAWITEM,wParam,lParam);

};



int ProcessCommandProxy(WPARAM wParam,LPARAM lParam) {
if (InternalGenMenuModule) 
{
	int val;
	val=CallService(MS_INT_MENUPROCESSCOMMAND,wParam,lParam);
	if (val) return(val);
};
 
return CallService(MS_CLIST_MENUPROCESSCOMMAND,wParam,lParam);

};

int ModifyMenuItemProxy(WPARAM wParam,LPARAM lParam) {
if (InternalGenMenuModule) 
{
	int val;
	val=CallService(MS_INT_MODIFYMENUITEM,wParam,lParam);
	if (val) return(val);
};
 
return CallService(MS_CLIST_MODIFYMENUITEM,wParam,lParam);

};


int InitFramesMenus(void)
{
	TMenuParam tmp;

if (!ServiceExists(MO_REMOVEMENUOBJECT))
{
	
	InitGenMenu();
	InitCustomMenus();
	InternalGenMenuModule=TRUE;
};

if (ServiceExists(MO_REMOVEMENUOBJECT))
{
	CreateServiceFunction("FrameMenuExecService",FrameMenuExecService);
	CreateServiceFunction("FrameMenuCheckService",FrameMenuCheckService);

	CreateServiceFunction(MS_CLIST_REMOVECONTEXTFRAMEMENUITEM,RemoveContextFrameMenuItem);
	CreateServiceFunction(MS_CLIST_ADDCONTEXTFRAMEMENUITEM,AddContextFrameMenuItem);
	CreateServiceFunction(MS_CLIST_MENUBUILDFRAMECONTEXT,BuildContextFrameMenu);
	CreateServiceFunction(MS_CLIST_FRAMEMENUNOTIFY,ContextFrameMenuNotify);
	hPreBuildFrameMenuEvent=CreateHookableEvent(ME_CLIST_PREBUILDFRAMEMENU);

	//frame menu object
	memset(&tmp,0,sizeof(tmp));
	tmp.cbSize=sizeof(tmp);
	tmp.CheckService="FrameMenuCheckService";
	tmp.ExecService="FrameMenuExecService";
	tmp.name="FrameMenu";
	hFrameMenuObject=CallService(MO_CREATENEWMENUOBJECT,0,(LPARAM)&tmp);
}
	return 0;
}
int UnitFramesMenu()
{
	/*
	if (ServiceExists(MO_REMOVEMENUOBJECT)) 
	{
		CallService(MO_REMOVEMENUOBJECT,hFrameMenuObject,0);
		if (InternalGenMenuModule){UnitGenMenu();};
	};
*/
	return(0);
};
