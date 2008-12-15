#include "commonheaders.h"

HANDLE hOnCntMenuBuild;
HGENMENU hMoveToGroupItem=0, hPriorityItem = 0, hFloatingItem = 0;
HANDLE *lphGroupsItems = NULL;
int nGroupsItems = 0, cbGroupsItems = 0;

//service
//wparam - hcontact
//lparam .popupposition from CLISTMENUITEM

#define MTG_MOVE "MoveToGroup/Move"

static TCHAR* PrepareGroupName( TCHAR* str )
{
	TCHAR* p = _tcschr( str, '&' ), *d;
	if ( p == NULL )
		return mir_tstrdup( str );

	d = p = ( TCHAR* )mir_alloc( sizeof( TCHAR )*( 2*_tcslen( str )+1 ));
	while ( *str ) {
		if ( *str == '&' )
			*d++ = '&';
		*d++ = *str++;
	}

	*d++ = 0;
	return p;
}

static HANDLE AddGroupItem(HGENMENU hRoot, TCHAR* name,int pos,int param,int checked)
{
	HANDLE result;
	CLISTMENUITEM mi = { 0 };
	mi.cbSize        = sizeof(mi);
	mi.hParentMenu   = hRoot;
	mi.popupPosition = param; // param to pszService - only with CMIF_CHILDPOPUP !!!!!!
	mi.position      = pos;
	mi.ptszName      = PrepareGroupName( name );
	mi.flags         = CMIF_ROOTHANDLE | CMIF_TCHAR | CMIF_KEEPUNTRANSLATED;
	if ( checked )
		mi.flags |= CMIF_CHECKED;
	mi.pszService = MTG_MOVE;
	result = ( HANDLE )CallService(MS_CLIST_ADDCONTACTMENUITEM, param, (LPARAM)&mi);
	mir_free( mi.ptszName );
	return result;
}

static void ModifyGroupItem(HANDLE hItem, TCHAR* name,int checked)
{
	CLISTMENUITEM mi = {0};
	mi.cbSize  = sizeof(mi);
	mi.ptszName = PrepareGroupName( name );
	mi.flags   = CMIM_NAME | CMIM_FLAGS | CMIF_TCHAR | CMIF_KEEPUNTRANSLATED;
	if ( checked )
		mi.flags |= CMIF_CHECKED;
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hItem, (LPARAM)&mi);
	mir_free( mi.ptszName );
}

static int OnContactMenuBuild(WPARAM wParam,LPARAM lParam)
{
	int i, pos;
	TCHAR *szGroupName, *szContactGroup;
	char intname[20];

	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof( mi );

	if ( !hMoveToGroupItem ) {
		mi.hParentMenu = NULL;
		mi.position = 100000;
		mi.pszName = LPGEN("&Move to Group");
		mi.flags = CMIF_ROOTHANDLE;
		hMoveToGroupItem = (HGENMENU)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
	}

	if ( !cbGroupsItems ) {
		cbGroupsItems = 0x10;
		lphGroupsItems = (HANDLE*)malloc(cbGroupsItems*sizeof(HANDLE));
	}

	szContactGroup = DBGetStringT((HANDLE)wParam, "CList", "Group");

	i=1;
	pos = 1000;
	if ( !nGroupsItems ) {
		nGroupsItems++;
		lphGroupsItems[0] = AddGroupItem(hMoveToGroupItem, TranslateT("<Root Group>"), pos++, -1, !szContactGroup);
	}
	else ModifyGroupItem( lphGroupsItems[0], TranslateT("<Root Group>"), !szContactGroup);

	pos += 100000; // Separator

	while (TRUE) {
		int checked;

		_itoa( i-1, intname, 10 );
		szGroupName = DBGetStringT( 0, "CListGroups", intname );

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
	CallService(MS_CLIST_CONTACTCHANGEGROUP, wParam, lParam < 0 ? 0 : lParam);
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
