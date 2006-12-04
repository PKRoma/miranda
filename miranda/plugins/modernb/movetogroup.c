#include "commonheaders.h"
#include "commonprototypes.h"

HANDLE hModulesLoaded=0,hOnCntMenuBuild=0;
HANDLE prevmenu=0;
HANDLE hMoveToGroupItem=0;
HANDLE *hGroupsItems = NULL;
int nGroupsItems = 0, cbGroupsItems = 0;

extern char *DBGetStringA(HANDLE hContact,const char *szModule,const char *szSetting);

HWND hwndTopToolBar=0;

//service
//wparam - hcontact
//lparam .popupposition from CLISTMENUITEM

#define MTG_MOVE								"MoveToGroup/Move"

static HANDLE AddGroupItem(HANDLE hRoot,char *name,int pos,int param,int checked)
{
	CLISTMENUITEM mi={0};

	mi.cbSize=sizeof(mi);
  mi.pszPopupName=(char *)hRoot;
  mi.popupPosition=param; // param to pszService - only with CMIF_CHILDPOPUP !!!!!!
	mi.position=pos;
  mi.pszName=name;
  mi.flags=CMIF_CHILDPOPUP;
  if (checked)
    mi.flags |= CMIF_CHECKED;
	mi.pszService=MTG_MOVE;
  return (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM,0, (LPARAM)&mi);
}

static void ModifyGroupItem(HANDLE hItem,char *name,int checked)
{
  CLISTMENUITEM mi={0};

  mi.cbSize=sizeof(mi);
  mi.pszName=name;
  mi.flags=CMIM_NAME | CMIM_FLAGS;
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
    mi.pszName=Translate("&Move to Group");
    mi.flags=CMIF_ROOTPOPUP;
    hMoveToGroupItem=(HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
  }

  {
    int i,pos;
    char *szGroupName,*szContactGroup;
    char intname[20];

    if (!cbGroupsItems)
    {
      cbGroupsItems = 0x10;
      hGroupsItems = (HANDLE*)malloc(cbGroupsItems*sizeof(HANDLE));
    }

    szContactGroup = DBGetStringA((HANDLE)wParam, "CList", "Group");

    i=1;
    pos = 1000;
    if (!nGroupsItems)
    {
      nGroupsItems++;
      hGroupsItems[0] = AddGroupItem(hMoveToGroupItem,Translate("Root Group"),pos++, -1, !szContactGroup);
    }
    else
      ModifyGroupItem(hGroupsItems[0],Translate("Root Group"), !szContactGroup);

    pos += 100000; // Separator


    while (TRUE)
    {
      int checked;

      _itoa(i-1,intname,10);
      szGroupName = DBGetStringA(0,"CListGroups",intname);

      if (!szGroupName || !strlen(szGroupName) || !szGroupName[0]) break;

      checked = 0;
      if (szContactGroup && !strcmp(szContactGroup, szGroupName + 1))
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
  char *grpname;
	char *intname;


	lParam--;

  if (lParam == -2) 
  { //root level
    DBDeleteContactSetting((HANDLE)wParam, "CList", "Group");

		return 0;
	}

  intname=(char *)_alloca(20);
	_itoa(lParam,intname,10);
  grpname=DBGetStringA(0,"CListGroups",intname);
  if (grpname)
	{
    DBWriteContactSettingString((HANDLE)wParam,"CList","Group",grpname + 1);
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

	return 0;
}	