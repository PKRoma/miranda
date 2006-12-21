#include "commonheaders.h"
#include "commonprototypes.h"

HANDLE hModulesLoaded=0,hOnCntMenuBuild=0;
HANDLE prevmenu=0;
HANDLE hMoveToGroupItem=0;
HANDLE *hGroupsItems = NULL;
int nGroupsItems = 0, cbGroupsItems = 0;
int LoadFavoriteContactMenu();
int UnloadFavoriteContactMenu();

//extern char *DBGetStringT(HANDLE hContact,const char *szModule,const char *szSetting);

HWND hwndTopToolBar=0;

//service
//wparam - hcontact
//lparam .popupposition from CLISTMENUITEM

#define MTG_MOVE								"MoveToGroup/Move"

static HANDLE AddGroupItem(HANDLE hRoot,TCHAR *name,int pos,int param,int checked)
{
    CLISTMENUITEM mi={0};

    mi.cbSize=sizeof(mi);
    mi.pszPopupName=(char *)hRoot;
    mi.popupPosition=param; // param to pszService - only with CMIF_CHILDPOPUP !!!!!!
    mi.position=pos;
    mi.ptszName=name;
    mi.flags=CMIF_CHILDPOPUP|CMIF_KEEPUNTRANSLATED|CMIF_TCHAR;
    if (checked)
        mi.flags |= CMIF_CHECKED;
    mi.pszService=MTG_MOVE;
    return (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,0, (LPARAM)&mi);
}

static void ModifyGroupItem(HANDLE hItem,TCHAR *name,int checked)
{
    CLISTMENUITEM mi={0};

    mi.cbSize=sizeof(mi);
    mi.ptszName=name;
    mi.flags=CMIM_NAME | CMIM_FLAGS|CMIF_KEEPUNTRANSLATED|CMIF_TCHAR;
    if (checked)
        mi.flags |= CMIF_CHECKED;
    CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hItem, (LPARAM)&mi);
}

static int OnContactMenuBuild(WPARAM wParam,LPARAM lParam)
{
    CLISTMENUITEM mi;

    if (!hMoveToGroupItem)
    {
        memset(&mi,0,sizeof(mi));
        mi.cbSize=sizeof(mi);
        mi.hIcon=NULL;//LoadIcon(hInst,MAKEINTRESOURCE(IDI_MIRANDA));
        mi.pszPopupName=(char *)-1;
        mi.position=100000;
        mi.pszName="&Move to Group";
        mi.flags=CMIF_ROOTPOPUP;
        hMoveToGroupItem=(HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
    }

    {
        int i,pos;
        TCHAR *szGroupName, *szContactGroup;
        char intname[20];

        if (!cbGroupsItems)
        {
            cbGroupsItems = 0x10;
            hGroupsItems = (HANDLE*)malloc(cbGroupsItems*sizeof(HANDLE));
        }

        szContactGroup = DBGetStringT((HANDLE)wParam, "CList", "Group");

        i=1;
        pos = 1000;
        if (!nGroupsItems)
        {
            nGroupsItems++;
            hGroupsItems[0] = AddGroupItem(hMoveToGroupItem,TranslateT("Root Group"),pos++, -1, !szContactGroup);
        }
        else
            ModifyGroupItem(hGroupsItems[0],TranslateT("Root Group"), !szContactGroup);

        pos += 100000; // Separator


        while (TRUE)
        {
            int checked;

            _itoa(i-1,intname,10);
            szGroupName = DBGetStringT(0,"CListGroups",intname);

            if (!szGroupName || !_tclen(szGroupName) || !szGroupName[0]) break;

            checked = 0;
            if (szContactGroup && !_tcsicmp(szContactGroup, szGroupName + 1))
                checked = 1;

            if (nGroupsItems > i)
                ModifyGroupItem(hGroupsItems[i], szGroupName + 1, checked);
            else
            {
                nGroupsItems++;
                if (cbGroupsItems < nGroupsItems)
                {
                    cbGroupsItems += 0x10;
                    hGroupsItems = (HANDLE*)realloc(hGroupsItems, cbGroupsItems*sizeof(HANDLE));
                }
                hGroupsItems[i] = AddGroupItem(hMoveToGroupItem,szGroupName + 1,pos++,i,checked);
            }
            i++;
            mir_free(szGroupName);
        }
        mir_free(szContactGroup);

        while (nGroupsItems > i)
        {
            nGroupsItems--;
            CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)hGroupsItems[nGroupsItems], 0);
            hGroupsItems[nGroupsItems] = NULL;
        }
    }
    return 0;
}


static int MTG_DOMOVE(WPARAM wParam,LPARAM lParam)
{
    TCHAR *grpname;
    char *intname;


    lParam--;

    if (lParam == -2) 
    { //root level
        DBDeleteContactSetting((HANDLE)wParam, "CList", "Group");

        return 0;
    }

    intname=(char *)_alloca(20);
    _itoa(lParam,intname,10);
    grpname=DBGetStringT(0,"CListGroups",intname);
    if (grpname)
    {
        DBWriteContactSettingTString((HANDLE)wParam,"CList","Group",grpname + 1);
        mir_free(grpname);
    };

    return 0;
}

static int MTG_OnmodulesLoad(WPARAM wParam,LPARAM lParam)
{
    if (!ServiceExists(MS_CLIST_REMOVECONTACTMENUITEM))
    {
        MessageBoxA(0,"New menu system not found - plugin disabled.","MoveToGroup",0);
        return 0;
    }

    hOnCntMenuBuild=HookEvent(ME_CLIST_PREBUILDCONTACTMENU,OnContactMenuBuild); 

    CreateServiceFunction(MTG_MOVE,MTG_DOMOVE);
    
    LoadFavoriteContactMenu();

    return 0;
}

int LoadMoveToGroup()
{
    hModulesLoaded=(HANDLE)HookEvent(ME_SYSTEM_MODULESLOADED,MTG_OnmodulesLoad);	
    return 0;
}

int UnloadMoveToGroup(void)
{
    UnhookEvent(hModulesLoaded);
    if (hOnCntMenuBuild)
        UnhookEvent(hOnCntMenuBuild);

    nGroupsItems = 0;
    cbGroupsItems = 0;
    free(hGroupsItems);
    UnloadFavoriteContactMenu();
    return 0;
}	


//////////////////////////////////////////////////////////////////////////
//
//  FAVORITE CONTACT SYSTEM
//
//////////////////////////////////////////////////////////////////////////

#define CLUI_FAVSETRATE "CLUI/SetContactRate"  //LParam is rate, Wparam is contact handle
#define CLUI_FAVTOGGLESHOWOFFLINE "CLUI/ToggleContactShowOffline" 

static HANDLE hFavoriteContactMenu=NULL;
static HANDLE *hFavoriteContactMenuItems = NULL;
static HANDLE hOnContactMenuBuild_FAV=NULL;

static int FAV_OnContactMenuBuild(WPARAM wParam,LPARAM lParam)
{
    CLISTMENUITEM mi;
    BOOL NeedFree=FALSE;
    BYTE bContactRate=DBGetContactSettingByte((HANDLE)wParam, "CList", "Rate",0);
    //if (hFavoriteContactMenu)
    TCHAR *rates[]={
            _T("None"),
            _T("Low"),
            _T("Medium"),
            _T("High") };
    
    char* iconsName[]={
                "Contact rate None",
                "Contact rate Low",
                "Contact rate Medium",
                "Contact rate High" };

    if (bContactRate>SIZEOF(rates)-1)
        bContactRate=SIZEOF(rates)-1;
    if (hFavoriteContactMenu)
        CallService(MO_REMOVEMENUITEM,(WPARAM)hFavoriteContactMenu,0);
    hFavoriteContactMenu=NULL;
    if (!hFavoriteContactMenu)
    {
        int i;
        TCHAR * name=NULL;
        #define FAVMENUROOTNAME _T("&Contact rate")

        memset(&mi,0,sizeof(mi));
        mi.cbSize=sizeof(mi);
        mi.hIcon=CLUI_LoadIconFromExternalFile("clisticons.dll",8,TRUE,TRUE,iconsName[bContactRate],"Contact List",Translate(iconsName[bContactRate]),-IDI_FAVORITE_0 - bContactRate, &NeedFree);
           // LoadSmallIcon(g_hInst,MAKEINTRESOURCE(IDI_FAVORITE_0 + bContactRate));
        mi.pszPopupName=(char *)-1;
        mi.position=0;
        if (!bContactRate)
            mi.ptszName=FAVMENUROOTNAME;
        else
        {
            int bufsize=(lstrlen(FAVMENUROOTNAME)+lstrlen(rates[bContactRate])+15)*sizeof(TCHAR);
            name=(TCHAR*)_alloca(bufsize);
            _sntprintf(name,bufsize/sizeof(TCHAR),_T("%s (%s)"),FAVMENUROOTNAME,rates[bContactRate]);
            mi.ptszName=name;            
        }
        mi.flags=CMIF_ROOTPOPUP|CMIF_TCHAR;
        hFavoriteContactMenu=(HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
        CallService(MS_SKIN2_RELEASEICON,(WPARAM)mi.hIcon,0);
        if (mi.hIcon && NeedFree) DestroyIcon(mi.hIcon);

        mi.pszPopupName=(char*)hFavoriteContactMenu;
        if (!hFavoriteContactMenuItems)
            hFavoriteContactMenuItems=(HANDLE)malloc(sizeof(HANDLE)*SIZEOF(rates));
        for (i=0; i<SIZEOF(rates); i++)
        {
            mi.hIcon=mi.hIcon=CLUI_LoadIconFromExternalFile("clisticons.dll",8+i,TRUE,TRUE,iconsName[i],"Contact List",Translate(iconsName[i]),-IDI_FAVORITE_0 - i, &NeedFree);
            mi.ptszName=rates[i];
            mi.flags=CMIF_CHILDPOPUP|CMIF_TCHAR|((bContactRate==i)?CMIF_CHECKED:0);
            mi.pszService=CLUI_FAVSETRATE;
            mi.popupPosition=i;
            hFavoriteContactMenuItems[i]=(HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
            CallService(MS_SKIN2_RELEASEICON,(WPARAM)mi.hIcon,0);
            if (mi.hIcon && NeedFree) DestroyIcon(mi.hIcon);
        }
        {
            mi.hIcon=NULL;
            mi.ptszName=_T("Show even if offline");
            mi.flags=CMIF_CHILDPOPUP|CMIF_TCHAR|(DBGetContactSettingByte((HANDLE)wParam,"CList","noOffline",0)?CMIF_CHECKED:0);
            mi.pszService=CLUI_FAVTOGGLESHOWOFFLINE;
            mi.popupPosition=i+100000000;
            mi.position=-100000000;
            CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);            
        }
    }
    return 0;
}

int FAV_SetRate(WPARAM hContact, LPARAM nRate)
{
    if (hContact)
    {
        DBWriteContactSettingByte((HANDLE)hContact, "CList", "Rate",(BYTE)nRate);
    }
    return 0;
}

int FAV_ToggleShowOffline(WPARAM hContact,LPARAM lParam)
{
   if (hContact)
   {
       DBWriteContactSettingByte((HANDLE)hContact,"CList","noOffline",
           DBGetContactSettingByte((HANDLE)hContact,"CList","noOffline",0)?0:1);
   }
   return 0;
}

int LoadFavoriteContactMenu()
{
    CreateServiceFunction(CLUI_FAVSETRATE,FAV_SetRate);
    CreateServiceFunction(CLUI_FAVTOGGLESHOWOFFLINE,FAV_ToggleShowOffline);
    hOnContactMenuBuild_FAV=HookEvent(ME_CLIST_PREBUILDCONTACTMENU,FAV_OnContactMenuBuild);
    return 0;
}

int UnloadFavoriteContactMenu()
{
    UnhookEvent(hOnContactMenuBuild_FAV);

    if (hFavoriteContactMenuItems)
        free (hFavoriteContactMenuItems);
    hFavoriteContactMenuItems=NULL;
    
    if (hFavoriteContactMenu)
        CallService(MO_REMOVEMENUITEM,(WPARAM)hFavoriteContactMenu,0);
    hFavoriteContactMenu=NULL;   

    return 0;
}


