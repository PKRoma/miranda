#include "commonheaders.h"
//#include "genmenu.h"

HANDLE hOnCntMenuBuild;
HANDLE hMoveToGroupItem=0, hPriorityItem = 0, hFloatingItem = 0;
HANDLE *lphGroupsItems = NULL;
int nGroupsItems = 0, cbGroupsItems = 0;

TCHAR* DBGetString(HANDLE hContact, const char* szModule, const char* szSetting)
{
	DBVARIANT dbv;
	if ( !DBGetContactSettingTString( hContact, szModule, szSetting, &dbv )) {
		TCHAR* str = mir_tstrdup( dbv.ptszVal );
		DBFreeVariant( &dbv );
		return str;
	}

	return NULL;
}

//service
//wparam - hcontact
//lparam .popupposition from CLISTMENUITEM

#define MTG_MOVE "MoveToGroup/Move"

static HANDLE AddGroupItem(HANDLE hRoot, TCHAR* name,int pos,int param,int checked)
{
	CLISTMENUITEM mi = { 0 };
	mi.cbSize        = sizeof(mi);
	mi.pszPopupName  = ( char* )hRoot;
	mi.popupPosition = param; // param to pszService - only with CMIF_CHILDPOPUP !!!!!!
	mi.position      = pos;
	mi.ptszName      = name;
	mi.flags         = CMIF_CHILDPOPUP | CMIF_TCHAR;
	if ( checked )
		mi.flags |= CMIF_CHECKED;
	mi.pszService = MTG_MOVE;
	return ( HANDLE )CallService(MS_CLIST_ADDCONTACTMENUITEM, param, (LPARAM)&mi);
}

static void ModifyGroupItem(HANDLE hItem, TCHAR* name,int checked)
{
	CLISTMENUITEM mi = {0};
	mi.cbSize  = sizeof(mi);
	mi.ptszName = name;
	mi.flags   = CMIM_NAME | CMIM_FLAGS | CMIF_TCHAR;
	if ( checked )
		mi.flags |= CMIF_CHECKED;
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hItem, (LPARAM)&mi);
}

static int OnContactMenuBuild(WPARAM wParam,LPARAM lParam)
{
	int i, pos;
	TCHAR *szGroupName, *szContactGroup;
	char intname[20];

	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof( mi );

	if ( !hMoveToGroupItem ) {
		mi.pszPopupName = ( char* )-1;
		mi.position = 100000;
		mi.pszName = "&Move to Group";
		mi.flags = CMIF_ROOTPOPUP;
		hMoveToGroupItem = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
	}

	if ( !cbGroupsItems ) {
		cbGroupsItems = 0x10;
		lphGroupsItems = (HANDLE*)malloc(cbGroupsItems*sizeof(HANDLE));
	}

	szContactGroup = DBGetString((HANDLE)wParam, "CList", "Group");

	i=1;
	pos = 1000;
	if ( !nGroupsItems ) {
		nGroupsItems++;
		lphGroupsItems[0] = AddGroupItem(hMoveToGroupItem, TranslateT("Root Group"), pos++, -1, !szContactGroup);
	}
	else ModifyGroupItem( lphGroupsItems[0], TranslateT("Root Group"), !szContactGroup);

	pos += 100000; // Separator

	while (TRUE) {
		int checked;

		_itoa( i-1, intname, 10 );
		szGroupName = DBGetString( 0, "CListGroups", intname );

		if ( !szGroupName || !szGroupName[0] )
			break;

		checked = 0;
		if ( szContactGroup && !_tcscmp( szContactGroup, szGroupName + 1 ))
			checked = 1;

		if ( nGroupsItems > i )
			ModifyGroupItem(lphGroupsItems[i], szGroupName + 1, checked);
		else {
			nGroupsItems++;
			if ( cbGroupsItems < nGroupsItems ) {
				cbGroupsItems += 0x10;
				lphGroupsItems = (HANDLE*)realloc( lphGroupsItems, cbGroupsItems*sizeof( HANDLE ));
			}
			lphGroupsItems[i] = AddGroupItem( hMoveToGroupItem, szGroupName + 1, pos++, i, checked );
		}
		i++;
		mir_free(szGroupName);
	}
	mir_free(szContactGroup);

	while (nGroupsItems > i) {
		nGroupsItems--;
		CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)lphGroupsItems[nGroupsItems], 0);
		lphGroupsItems[nGroupsItems] = NULL;
	}

	return 0;
}

static int MTG_DOMOVE(WPARAM wParam,LPARAM lParam)
{
	TCHAR* grpname;
	char* intname;

	lParam--;

	//root level
	if ( lParam == -2 ) {
		DBDeleteContactSetting((HANDLE)wParam, "CList", "Group");
		return 0;
	}

	intname=(char *)_alloca(20);
	_itoa(lParam,intname,10);
	grpname = DBGetString(0,"CListGroups",intname);
	if ( grpname ) {
		DBWriteContactSettingTString((HANDLE)wParam,"CList","Group",grpname + 1);
		mir_free(grpname);
	}

	return 0;
}

void MTG_OnmodulesLoad()
{
	hOnCntMenuBuild=HookEvent(ME_CLIST_PREBUILDCONTACTMENU,OnContactMenuBuild); 
	CreateServiceFunction(MTG_MOVE,MTG_DOMOVE);
}

int UnloadMoveToGroup(void)
{
	if (hOnCntMenuBuild)
		UnhookEvent(hOnCntMenuBuild);

	nGroupsItems = 0;
	cbGroupsItems = 0;
	free(lphGroupsItems);
	return 0;
}
