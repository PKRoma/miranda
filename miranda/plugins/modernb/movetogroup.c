#include "commonheaders.h"

HANDLE hModulesLoaded,hOnCntMenuBuild;
HANDLE prevmenu=0;
extern char *DBGetStringA(HANDLE hContact,const char *szModule,const char *szSetting);

HWND hwndTopToolBar=0;

//service
//wparam - hcontact
//lparam .popupposition from CLISTMENUITEM

#define MTG_MOVE								"MoveToGroup/Move"

static int AddGroupItem(int rootid,TCHAR *name,int pos,int poppos,WPARAM wParam)
{
	CLISTMENUITEM mi={0};
	mi.cbSize=sizeof(mi);
	mi.hIcon=NULL;//LoadSmallIconShared(hInst,MAKEINTRESOURCEA(IDI_MIRANDA));
	mi.pszPopupName=(char *)rootid;
	mi.popupPosition=poppos;
	mi.position=pos;
	mi.ptszName=name;
	mi.flags=CMIF_CHILDPOPUP|CMIF_TCHAR;
	mi.pszContactOwner=(char *)0;
	mi.pszService=MTG_MOVE;
	return(CallService(MS_CLIST_ADDCONTACTMENUITEM,wParam,(LPARAM)&mi));

}

static int OnContactMenuBuild(WPARAM wParam,LPARAM lParam)
{

	CLISTMENUITEM mi;
	HANDLE menuid;
	int i,grpid;
	boolean grpexists;
	TCHAR *grpname;
	char intname[20];
    if (MirandaExiting()) return 0;
	if (prevmenu!=0){
		CallService(MS_CLIST_REMOVECONTACTMENUITEM,(WPARAM)prevmenu,(LPARAM)0);
	};
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.hIcon=NULL;//LoadSmallIconShared(hInst,MAKEINTRESOURCEA(IDI_MIRANDA));
	mi.pszPopupName=(char *)-1;
	mi.position=100000;
	mi.ptszName=TranslateT("&Move to Group");
	mi.flags=CMIF_ROOTPOPUP|CMIF_TCHAR;
	mi.pszContactOwner=(char *)0;
	menuid=(HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,wParam,(LPARAM)&mi);
	prevmenu=menuid;
	grpexists=TRUE;
	i=0;
	grpid=1000;
	AddGroupItem((int)menuid,TranslateT("Root Group"),grpid++,-1,wParam);
	grpid+=100000;
	while (TRUE) 
	{
		_itoa(i,intname,10);
		grpname=DBGetStringT(0,"CListGroups",intname);
		if (grpname==NULL ){break;};
		if (lstrlen(grpname)==0) {break; };
		if (grpname[0]==0) {break; };
		AddGroupItem((int)menuid,&(grpname[1]),grpid++,i+1,wParam);
		i++;
		mir_free_and_nill(grpname);
	};
	return 0;
};

static int MTG_DOMOVE(WPARAM wParam,LPARAM lParam)
{
	TCHAR *grpname,*correctgrpname;
	char *intname;
	if (lParam==0)
	{
		MessageBoxA(0,"Wrong version of New menu system - please update.","MoveToGroup",0);
		return(0);
	};
	lParam--;
	if (lParam==-2)//root level
	{
		DBWriteContactSettingString((HANDLE)wParam,"CList","Group","");
		return 0;
	}
	intname=(char *)malloc(20);
	_itoa(lParam,intname,10);
	grpname=DBGetStringT(0,"CListGroups",intname);
	if (grpname!=0)
	{
		correctgrpname=&(grpname[1]);
		DBWriteContactSettingTString((HANDLE)wParam,"CList","Group",correctgrpname);
		mir_free_and_nill(grpname);
	};

	free (intname);
	return 0;
};
static int OnmodulesLoad(WPARAM wParam,LPARAM lParam)
{
	if (!ServiceExists(MS_CLIST_REMOVECONTACTMENUITEM))
	{
		MessageBoxA(0,"New menu system not found - plugin disabled.","MoveToGroup",0);
		return(0);
	};
	hOnCntMenuBuild=(HANDLE)HookEvent(ME_CLIST_PREBUILDCONTACTMENU,OnContactMenuBuild);	
	CreateServiceFunction(MTG_MOVE,MTG_DOMOVE);
	return(0);
};


int LoadMoveToGroup()
{
	hModulesLoaded=(HANDLE)HookEvent(ME_SYSTEM_MODULESLOADED,OnmodulesLoad);	
	return 0;
}

int UnloadMoveToGroup(void)
{
	UnhookEvent(hModulesLoaded);
	return 0;
}	