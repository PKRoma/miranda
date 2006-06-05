#include "../commonheaders.h"

HANDLE hModulesLoaded,hOnCntMenuBuild;
HANDLE prevmenu=0, hPriorityItem = 0, hFloatingItem = 0, hIgnoreItem = 0;
extern char *DBGetString(HANDLE hContact,const char *szModule,const char *szSetting);

HWND hwndTopToolBar=0;

//service
//wparam - hcontact
//lparam .popupposition from CLISTMENUITEM

#define MTG_MOVE								"MoveToGroup/Move"

static int AddGroupItem(int rootid,char *name,int pos,int poppos,WPARAM wParam)
{
    CLISTMENUITEM mi={0};


    mi.cbSize=sizeof(mi);
    mi.hIcon=NULL;//LoadIcon(hInst,MAKEINTRESOURCE(IDI_MIRANDA));
    mi.pszPopupName=(char *)rootid;
    mi.popupPosition=poppos;
    mi.position=pos;
    mi.pszName=name;
    mi.flags=CMIF_CHILDPOPUP;
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
    char *grpname;
    char intname[20];

    if (prevmenu!=0) {
        CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)prevmenu,(LPARAM)0);
		CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)hPriorityItem, 0);
		CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)hFloatingItem, 0);
        CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)hIgnoreItem, 0);
    }

	if(DBGetContactSettingByte(0, "CList", "flt_enabled", 0)) {
		memset(&mi,0,sizeof(mi));
		mi.cbSize=sizeof(mi);
		mi.position=200000;
		mi.pszPopupName=(char *)-1;
		mi.pszService="CList/SetContactFloating";
		mi.pszName=Translate("&Floating Contact");
		if(pcli) {
			if(SendMessage(pcli->hwndContactTree, CLM_QUERYFLOATINGCONTACT, wParam, 0))
				mi.flags=CMIF_CHECKED;
		}
		mi.pszContactOwner=(char *)0;
		hFloatingItem = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,wParam,(LPARAM)&mi);
	}

	memset(&mi,0,sizeof(mi));
    mi.cbSize=sizeof(mi);
    mi.position=200000;
    mi.pszPopupName=(char *)-1;
    mi.pszService="CList/SetContactPriority";
    mi.pszName=Translate("&Priority Contact");
	if(pcli) {
		if(SendMessage(pcli->hwndContactTree, CLM_QUERYPRIORITYCONTACT, wParam, 0))
			mi.flags=CMIF_CHECKED;
	}
    mi.pszContactOwner=(char *)0;
    hPriorityItem = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,wParam,(LPARAM)&mi);

    memset(&mi,0,sizeof(mi));
    mi.cbSize=sizeof(mi);
    mi.position=200000;
    mi.pszPopupName=(char *)-1;
    mi.pszService="CList/SetContactIgnore";
    mi.pszName=Translate("&Visibility and ignore...");
    mi.pszContactOwner=(char *)0;
    hIgnoreItem = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,wParam,(LPARAM)&mi);


    ZeroMemory(&mi,sizeof(mi));
    mi.cbSize=sizeof(mi);
    mi.hIcon=NULL;//LoadIcon(hInst,MAKEINTRESOURCE(IDI_MIRANDA));
    mi.pszPopupName=(char *)-1;
    mi.position=100000;
    mi.pszName=Translate("&Move to Group");
    mi.flags=CMIF_ROOTPOPUP;
    mi.pszContactOwner=(char *)0;
    menuid=(HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,wParam,(LPARAM)&mi);
	prevmenu=menuid;

    grpexists=TRUE;
    i=0;
//	intname=(char *)malloc(20);
    grpid=1000;
    AddGroupItem((int)menuid,Translate("Root Group"),grpid++,-1,wParam);
    grpid+=100000;
    //AddGroupItem(menuid,"---------------------------------------------",grpid++,0);
    while (TRUE) {
        _itoa(i,intname,10);
        grpname=DBGetString(0,"CListGroups",intname);

        if (grpname==NULL ) {
            break;
        };
        if (strlen(grpname)==0) {

            break;
        };
        if (grpname[0]==0) {

            break;
        };
        AddGroupItem((int)menuid,&(grpname[1]),grpid++,i+1,wParam);
    /*
    mi.cbSize=sizeof(mi);
    mi.hIcon=NULL;//LoadIcon(hInst,MAKEINTRESOURCE(IDI_MIRANDA));
    mi.pszPopupName=(char *)menuid;
    mi.popupPosition=i+1;
    mi.position=grpid++;
    mi.pszName=&(grpname[1]);
    mi.flags=CMIF_CHILDPOPUP;
    mi.pszContactOwner=(char *)0;
    mi.pszService=MTG_MOVE;
    CallService(MS_CLIST_ADDCONTACTMENUITEM,wParam,(LPARAM)&mi);
    */
        i++;
        mir_free(grpname);
    };
    return 0;
};

static int MTG_DOMOVE(WPARAM wParam,LPARAM lParam)
{
    char *grpname,*correctgrpname;
    char *intname;


    lParam--;

    if (lParam == -2) {//root level
        DBDeleteContactSetting((HANDLE)wParam, "CList", "Group");
        //DBWriteContactSettingString((HANDLE)wParam,"CList","Group","");
        return 0;
    }
    intname=(char *)malloc(20);
    _itoa(lParam,intname,10);
    grpname=DBGetString(0,"CListGroups",intname);
    if (grpname!=0) {
        correctgrpname=&(grpname[1]);
        DBWriteContactSettingString((HANDLE)wParam,"CList","Group",correctgrpname);
        mir_free(grpname);
    };


    free (intname);
    return 0;
}

int MTG_OnmodulesLoad(WPARAM wParam,LPARAM lParam)
{
    if (!ServiceExists(MS_CLIST_REMOVECONTACTMENUITEM)) {
        MessageBoxA(0,"New menu system not found - plugin disabled.","MoveToGroup",0);
        return(0);
    };
    hOnCntMenuBuild=HookEvent(ME_CLIST_PREBUILDCONTACTMENU,OnContactMenuBuild); 
    CreateServiceFunction(MTG_MOVE,MTG_DOMOVE);
    return(0);
};


/*
int LoadMoveToGroup()
{

    hModulesLoaded=HookEvent(ME_SYSTEM_MODULESLOADED,OnmodulesLoad);	
    
    return 0;
}*/

int UnloadMoveToGroup(void)
{
    //if (hFrameTopWindow!=NULL){CallService(MS_CLIST_FRAMES_REMOVEFRAME,(WPARAM)hFrameTopWindow,(LPARAM)0);};

    //DestroyServiceFunction();
    UnhookEvent(hModulesLoaded);

    return 0;
}   