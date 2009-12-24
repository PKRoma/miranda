#include "commonheaders.h"

HANDLE hOnCntMenuBuild;
HGENMENU hMoveToGroupItem=0, hPriorityItem = 0, hFloatingItem = 0;

LIST<HANDLE> lphGroupsItems(5);

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

static void AddGroupItem(HGENMENU hRoot, TCHAR* name, int pos, WPARAM param, bool checked)
{
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
	HANDLE result = ( HANDLE )CallService(MS_CLIST_ADDCONTACTMENUITEM, param, (LPARAM)&mi);
	mir_free( mi.ptszName );

	lphGroupsItems.insert((HANDLE*)result);
}

static int OnContactMenuBuild(WPARAM wParam,LPARAM)
{
	int i;

	if (!hMoveToGroupItem) 
	{
		CLISTMENUITEM mi = {0};

		mi.cbSize = sizeof(mi);
		mi.position = 100000;
		mi.pszName = LPGEN("&Move to Group");
		mi.flags = CMIF_ROOTHANDLE | CMIF_ICONFROMICOLIB;
		mi.icolibItem = GetSkinIconHandle(SKINICON_OTHER_GROUP);

		hMoveToGroupItem = (HGENMENU)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
	}

	for (i = 0; i < lphGroupsItems.getCount(); i++)
		CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)lphGroupsItems[i], 0);
	lphGroupsItems.destroy();

	TCHAR *szContactGroup = DBGetStringT((HANDLE)wParam, "CList", "Group");

	int pos = 1000;

	AddGroupItem(hMoveToGroupItem, TranslateT("<Root Group>"), pos, -1, !szContactGroup);

	pos += 100000; // Separator

	for (i = 0; ; ++i) 
	{
		char intname[20];
		_itoa(i, intname, 10);

		DBVARIANT dbv;
		if (DBGetContactSettingTString(NULL, "CListGroups", intname, &dbv))
			break;

		if (dbv.ptszVal[0])
		{
			TCHAR* szGroupName = dbv.ptszVal + 1;
			bool checked = szContactGroup && !_tcscmp(szContactGroup, szGroupName);
			AddGroupItem(hMoveToGroupItem, szGroupName, ++pos, i + 1, checked);
		}
		mir_free(dbv.ptszVal);
	}
	mir_free(szContactGroup);

	return 0;
}

static INT_PTR MTG_DOMOVE(WPARAM wParam,LPARAM lParam)
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
	UnhookEvent(hOnCntMenuBuild);
	lphGroupsItems.destroy();

	return 0;
}
