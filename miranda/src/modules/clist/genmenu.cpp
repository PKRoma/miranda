#include "commonheaders.h"
#include "genmenu.h"

static boolean bIsGenMenuInited = FALSE;
static CRITICAL_SECTION csMenuHook;

static int NextObjectId = 0x100, NextObjectMenuItemId = CLISTMENUIDMIN;

static int CompareMenus( const TIntMenuObject* p1, const TIntMenuObject* p2 )
{
	return lstrcmpA( p1->Name, p2->Name );
}

LIST<TIntMenuObject> g_menus( 10, CompareMenus );

void FreeAndNil( void **p )
{
	if ( p == NULL )
		return;

	if ( *p != NULL ) {
		mir_free( *p );
		*p = NULL;
}	}

int GetMenuObjbyId( const int id )
{
	int i;
	for ( i=0; i < g_menus.getCount(); i++ )
		if ( g_menus[i]->id == id )
			return i;

	return -1;
}

int GetMenuItembyId( const int objpos, const int id )
{
	if ( objpos < 0 || objpos >= g_menus.getCount() )
		return -1;

	for ( int i=0; i < g_menus[objpos]->m_items.getCount(); i++ )
		if ( g_menus[objpos]->m_items[i]->id == id )
			return i;

	return -1;
}

PMO_IntMenuItem GetMenuItemByGlobalID( int globalMenuID )
{
	int pimoidx = GetMenuObjbyId( HIWORD( globalMenuID ));
	int itempos = GetMenuItembyId( pimoidx, LOWORD( globalMenuID ));

	if ( pimoidx >= 0 && itempos >= 0 )
		if ( pimoidx < g_menus.getCount() && itempos < g_menus[pimoidx]->m_items.getCount() )
			if ( g_menus[pimoidx]->m_items[itempos]->globalid == globalMenuID )
				return g_menus[pimoidx]->m_items[itempos];

	return NULL;
}

int getGlobalId(const int MenuObjectId,const int MenuItemId)
{
	return ( MenuObjectId << 16 ) + MenuItemId;
}

int GetObjIdxByItemId(const int MenuItemId)
{
	for ( int i=0; i < g_menus.getCount(); i++ ) {
		TIntMenuObject* pmo = g_menus[i];
		for ( int j=0; j < pmo->m_items.getCount(); j++ )
			if ( pmo->m_items[j]->id == MenuItemId )
				return i;
	}

	return -1;
}

//return 0-erorr,1-ok
int GetAllIdx(const int globid,int *pobjidx,int *pmenuitemidx)
{
	int objid,menuitemid;//id
	int objidx,menuitemidx;//pos in array

	if ( pobjidx == NULL || pmenuitemidx == NULL)
		return 0;

	UnpackGlobalId( globid, &objid, &menuitemid );
	if ( objid == -1 || menuitemid == -1 )
		return 0;

	objidx = GetMenuObjbyId( objid );
	menuitemidx = GetMenuItembyId( objidx, menuitemid );
	if ( objidx == -1 || menuitemidx == -1 )
		return 0;

	*pobjidx = objidx;
	*pmenuitemidx = menuitemidx;
	return 1;
}

void UnpackGlobalId( int id, int* MenuObjectId, int* MenuItemId )
{
	if ( MenuObjectId == NULL || MenuItemId == NULL )
		return;

	*MenuObjectId = ( id >> 16 );
	*MenuItemId   = id & 0xFFFF;
}

//wparam=0
//lparam=LPMEASUREITEMSTRUCT
int MO_MeasureMenuItem( LPMEASUREITEMSTRUCT mis )
{
	PMO_IntMenuItem pimi = NULL;

	// prevent win9x from ugly menus displaying when there is no icon
	mis->itemWidth = 0;
	mis->itemHeight = 0;

	if ( !bIsGenMenuInited )
		return -1;

	if ( mis == NULL )
		return FALSE;

	pimi = MO_GetIntMenuItem( mis->itemData );
	if ( pimi == NULL )
		return FALSE;

	if ( pimi->iconId == -1 )
		return FALSE;

	mis->itemWidth = max(0,GetSystemMetrics(SM_CXSMICON)-GetSystemMetrics(SM_CXMENUCHECK)+4);
	mis->itemHeight = GetSystemMetrics(SM_CYSMICON)+2;
	return TRUE;
}

//wparam=0
//lparam=LPDRAWITEMSTRUCT
int MO_DrawMenuItem( LPDRAWITEMSTRUCT dis )
{
	PMO_IntMenuItem pimi = NULL;
	int y,objidx,menuitemidx;

	if ( !bIsGenMenuInited )
		return -1;

	if ( dis == NULL )
		return FALSE;

	EnterCriticalSection( &csMenuHook );

	if ( GetAllIdx( dis->itemData, &objidx, &menuitemidx ) == 0 ) {
		LeaveCriticalSection( &csMenuHook );
		return FALSE;
	}

	pimi = g_menus[objidx]->m_items[menuitemidx];
	if ( pimi == NULL || pimi->iconId == -1 ) {
		LeaveCriticalSection( &csMenuHook );
		return FALSE;
	}

	y = (dis->rcItem.bottom - dis->rcItem.top - GetSystemMetrics(SM_CYSMICON))/2+1;
	if ( dis->itemState & ODS_SELECTED ) {
		if ( dis->itemState & ODS_CHECKED ) {
			RECT rc;
			rc.left = 2; rc.right = GetSystemMetrics(SM_CXSMICON)+2;
			rc.top = y; rc.bottom = rc.top+GetSystemMetrics(SM_CYSMICON)+2;
			FillRect(dis->hDC, &rc, GetSysColorBrush( COLOR_HIGHLIGHT ));
			ImageList_DrawEx( g_menus[objidx]->m_hMenuIcons, pimi->iconId, dis->hDC, 2, y, 0, 0, CLR_NONE, CLR_DEFAULT, ILD_SELECTED );
		}
		else ImageList_DrawEx( g_menus[objidx]->m_hMenuIcons, pimi->iconId, dis->hDC, 2, y, 0, 0, CLR_NONE, CLR_DEFAULT, ILD_FOCUS );
	}
	else {
		if ( dis->itemState&ODS_CHECKED) {
			HBRUSH hBrush;
			RECT rc;
			COLORREF menuCol,hiliteCol;
			rc.left = 0; rc.right = GetSystemMetrics(SM_CXSMICON)+4;
			rc.top = y-2; rc.bottom = rc.top + GetSystemMetrics(SM_CYSMICON)+4;
			DrawEdge(dis->hDC,&rc,BDR_SUNKENOUTER,BF_RECT);
			InflateRect(&rc,-1,-1);
			menuCol = GetSysColor(COLOR_MENU);
			hiliteCol = GetSysColor(COLOR_3DHIGHLIGHT);
			hBrush = CreateSolidBrush(RGB((GetRValue(menuCol)+GetRValue(hiliteCol))/2,(GetGValue(menuCol)+GetGValue(hiliteCol))/2,(GetBValue(menuCol)+GetBValue(hiliteCol))/2));
			FillRect(dis->hDC,&rc,GetSysColorBrush(COLOR_MENU));
			DeleteObject(hBrush);
			ImageList_DrawEx(g_menus[objidx]->m_hMenuIcons,pimi->iconId,dis->hDC,2,y,0,0,CLR_NONE,GetSysColor(COLOR_MENU),ILD_BLEND50);
		}
		else ImageList_DrawEx(g_menus[objidx]->m_hMenuIcons,pimi->iconId,dis->hDC,2,y,0,0,CLR_NONE,CLR_NONE,ILD_NORMAL);
	}
	LeaveCriticalSection( &csMenuHook );
	return TRUE;
}

int MO_RemoveAllObjects()
{
	int i;
	for ( i=0; i < g_menus.getCount(); i++ )
		delete g_menus[i];

	g_menus.destroy();
	return 0;
}

//wparam=MenuObjectHandle
int MO_RemoveMenuObject(WPARAM wParam,LPARAM lParam)
{
	int objidx;

	if ( !bIsGenMenuInited) return -1;
	EnterCriticalSection( &csMenuHook );

	objidx = GetMenuObjbyId(( int )wParam );
	if ( objidx == -1 ) {
		LeaveCriticalSection( &csMenuHook );
		return -1;
	}

	delete g_menus[ objidx ];
	g_menus.remove( objidx );
	LeaveCriticalSection( &csMenuHook );
	return 0;
}

//wparam=MenuObjectHandle
//lparam=vKey
int MO_ProcessHotKeys( int menuHandle, int vKey )
{
	int objidx;//pos in array
	int i;

	if ( !bIsGenMenuInited)
		return -1;

	EnterCriticalSection( &csMenuHook );

	objidx = GetMenuObjbyId( menuHandle );
	if ( objidx == -1 ) {
		LeaveCriticalSection( &csMenuHook );
		return FALSE;
	}

	for ( i=0; i < g_menus[objidx]->m_items.getCount(); i++ ) {
		PMO_IntMenuItem pimi = g_menus[objidx]->m_items[i];
		if ( pimi->mi.hotKey == 0 ) continue;
		if ( HIWORD(pimi->mi.hotKey) != vKey) continue;
		if ( !(LOWORD(pimi->mi.hotKey) & MOD_ALT     ) != !( GetKeyState( VK_MENU    ) & 0x8000)) continue;
		if ( !(LOWORD(pimi->mi.hotKey) & MOD_CONTROL ) != !( GetKeyState( VK_CONTROL ) & 0x8000)) continue;
		if ( !(LOWORD(pimi->mi.hotKey) & MOD_SHIFT   ) != !( GetKeyState( VK_SHIFT   ) & 0x8000)) continue;

		MO_ProcessCommand(( WPARAM )getGlobalId(g_menus[objidx]->id, pimi->id ), 0 );
		LeaveCriticalSection( &csMenuHook );
		return TRUE;
	}

	LeaveCriticalSection( &csMenuHook );
	return FALSE;
}

//wparam=MenuItemHandle
//lparam=PMO_MenuItem
int MO_GetMenuItem(WPARAM wParam,LPARAM lParam)
{
	PMO_IntMenuItem pimi;
	PMO_MenuItem mi=(PMO_MenuItem)lParam;

	if ( !bIsGenMenuInited || mi == NULL )
		return -1;

	EnterCriticalSection( &csMenuHook );
	pimi = MO_GetIntMenuItem(wParam);
	if ( pimi != NULL ) {
		*mi = pimi->mi;
		LeaveCriticalSection( &csMenuHook );
		return 0;
	}
	LeaveCriticalSection( &csMenuHook );
	return -1;
}

//wparam MenuItemHandle
//lparam PMO_MenuItem
int MO_ModifyMenuItem( int menuHandle, PMO_MenuItem pmi )
{
	PMO_IntMenuItem pimi;
	int oldflags;
	int objidx;

	if ( !bIsGenMenuInited || pmi == NULL )
		return -1;

	EnterCriticalSection( &csMenuHook );
	pimi = MO_GetIntMenuItem( menuHandle );
	if ( pimi == NULL || pmi->cbSize != sizeof( TMO_MenuItem )) {
		LeaveCriticalSection( &csMenuHook );
		return -1;
	}

	objidx = GetObjIdxByItemId(pimi->id);
	if ( objidx == -1 ) {
		LeaveCriticalSection( &csMenuHook );
		return -1;
	}

	if ( pmi->flags & CMIM_NAME ) {
		FreeAndNil(( void** )&pimi->mi.pszName );
#if defined( _UNICODE )
		if ( pmi->flags & CMIF_UNICODE )
			pimi->mi.ptszName = mir_tstrdup(( pmi->flags & CMIF_KEEPUNTRANSLATED ) ? pmi->ptszName : TranslateTS( pmi->ptszName ));
		else {
			if ( pmi->flags & CMIF_KEEPUNTRANSLATED ) {
				int len = lstrlenA( pmi->pszName );
				pimi->mi.ptszName = ( TCHAR* )mir_alloc( sizeof( TCHAR )*( len+1 ));
				MultiByteToWideChar( CP_ACP, 0, pmi->pszName, -1, pimi->mi.ptszName, len+1 );
				pimi->mi.ptszName[ len ] = 0;
			}
			else pimi->mi.ptszName = LangPackPcharToTchar( pmi->pszName );
		}
#else
		pimi->mi.ptszName = mir_strdup(( pmi->flags & CMIF_KEEPUNTRANSLATED ) ? pmi->ptszName :  Translate( pmi->ptszName ));
#endif
	}
	if ( pmi->flags & CMIM_FLAGS ) {
		oldflags = pimi->mi.flags & ( CMIF_ROOTPOPUP | CMIF_CHILDPOPUP | CMIF_ICONFROMICOLIB );
		pimi->mi.flags = pmi->flags & ~CMIM_ALL;
		pimi->mi.flags |= oldflags;
	}
	if ( pmi->flags & CMIM_ICON ) {
		if ( pimi->mi.flags & CMIF_ICONFROMICOLIB ) {
			HICON hIcon = IcoLib_GetIconByHandle( pmi->hIcolibItem );
			if ( hIcon != NULL ) {
				pimi->hIcolibItem = pmi->hIcolibItem;
				pimi->iconId = ImageList_ReplaceIcon( g_menus[objidx]->m_hMenuIcons, pimi->iconId, hIcon );
				IconLib_ReleaseIcon( hIcon, 0 );
			}
			else pimi->iconId = -1, pimi->hIcolibItem = NULL;
		}
		else {
			pimi->mi.hIcon = pmi->hIcon;
			if ( pmi->hIcon != NULL )
				pimi->iconId = ImageList_ReplaceIcon( g_menus[objidx]->m_hMenuIcons, pimi->iconId, pmi->hIcon );
			else
				pimi->iconId = -1;	  //fixme, should remove old icon & shuffle all iconIds
		}	}

	if ( pmi->flags & CMIM_HOTKEY )
		pimi->mi.hotKey = pmi->hotKey;

	LeaveCriticalSection( &csMenuHook );
	return 0;
}

//wparam MenuItemHandle
//return ownerdata useful to free ownerdata before delete menu item,
//NULL on error.
int MO_MenuItemGetOwnerData(WPARAM wParam,LPARAM lParam)
{
	if ( !bIsGenMenuInited )
		return -1;

	EnterCriticalSection( &csMenuHook );

	int objid,menuitemid;//id
	UnpackGlobalId( wParam, &objid, &menuitemid );
	if ( objid == -1 || menuitemid == -1 ) {
		LeaveCriticalSection( &csMenuHook );
		return 0;
	}

	int objidx = GetMenuObjbyId( objid );
	int menuitemidx = GetMenuItembyId( objidx, menuitemid );
	if ( objidx == -1 || menuitemidx == -1 ) {
		LeaveCriticalSection( &csMenuHook );
		return 0;
	}

	int res = (int)g_menus[objidx]->m_items[menuitemidx]->mi.ownerdata;
	LeaveCriticalSection( &csMenuHook );
	return res;
}

PMO_IntMenuItem MO_GetIntMenuItem(int globid)
{
	int objid,menuitemid;//id
	int objidx,menuitemidx;//pos in array

	UnpackGlobalId( globid, &objid, &menuitemid );
	if ( objid == -1 || menuitemid == -1 )
		return 0;

	objidx = GetMenuObjbyId( objid );
	menuitemidx = GetMenuItembyId( objidx, menuitemid );
	if ( objidx == -1 || menuitemidx == -1 )
		return 0;

	return g_menus[objidx]->m_items[menuitemidx];
}

//LOWORD(wparam) menuident
int MO_ProcessCommandByMenuIdent(WPARAM wParam,LPARAM lParam)
{
	int i,j;

	if ( !bIsGenMenuInited )
		return -1;

	EnterCriticalSection( &csMenuHook );
	for ( i=0; i < g_menus.getCount(); i++ ) {
		for ( j=0; j < g_menus[i]->m_items.getCount(); j++ ) {
			if ( g_menus[i]->m_items[j]->id == (int)wParam ) {
				int globid = getGlobalId( g_menus[i]->id, g_menus[i]->m_items[j]->id );
				LeaveCriticalSection( &csMenuHook );
				return MO_ProcessCommand(globid,lParam);
	}	}	}

	LeaveCriticalSection( &csMenuHook );
	return FALSE;
}

int MO_ProcessCommand(WPARAM wParam,LPARAM lParam)
{
	int objidx,menuitemidx;//pos in array
	int globid=wParam;
	char *srvname;
	void *ownerdata;

	if ( !bIsGenMenuInited )
		return -1;

	EnterCriticalSection( &csMenuHook );
	if ( GetAllIdx( globid, &objidx, &menuitemidx ) == 0 ) {
		LeaveCriticalSection( &csMenuHook );
		return 0;
	}
	srvname = g_menus[objidx]->ExecService;
	ownerdata = g_menus[objidx]->m_items[menuitemidx]->mi.ownerdata;
	LeaveCriticalSection( &csMenuHook );
	CallService( srvname, ( WPARAM )ownerdata, lParam );
	return 1;
}

int MO_SetOptionsMenuItem( int handle, int setting, int value )
{
	int objidx;
	int res = -1;

	if ( !bIsGenMenuInited )
		return res;

	EnterCriticalSection( &csMenuHook );
	__try 
	{
		PMO_IntMenuItem pimi = MO_GetIntMenuItem( handle );
		if ( pimi != NULL ) {
			objidx = GetObjIdxByItemId( pimi->id );
			if ( objidx != -1 ) {
				res = 1;
				if ( setting == OPT_MENUITEMSETUNIQNAME ) {
					mir_free( pimi->UniqName );
					pimi->UniqName = mir_strdup(( char* )value );
				}
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER ) {}

	LeaveCriticalSection( &csMenuHook );
	return res;
}

int MO_SetOptionsMenuObject( int handle, int setting, int value )
{
	int  pimoidx;
	int  res = 0;

	if ( !bIsGenMenuInited )
		return -1;

	EnterCriticalSection( &csMenuHook );
	__try 
	{
		pimoidx = GetMenuObjbyId( handle );
		res = pimoidx != -1;
		if ( res ) {
			TIntMenuObject* pmo = g_menus[pimoidx];

			switch ( setting ) {
			case OPT_MENUOBJECT_SET_ONADD_SERVICE:
				FreeAndNil(( void** )&pmo->onAddService );
				pmo->onAddService = mir_strdup(( char* )value );
				break;

			case OPT_MENUOBJECT_SET_FREE_SERVICE:
				FreeAndNil(( void** )&pmo->FreeService );
				pmo->FreeService = mir_strdup(( char* )value );
				break;

			case OPT_MENUOBJECT_SET_CHECK_SERVICE:
				FreeAndNil(( void** )&pmo->CheckService );
				pmo->CheckService = mir_strdup(( char* )value);
				break;

			case OPT_USERDEFINEDITEMS:
				pmo->m_bUseUserDefinedItems = ( BOOL )value;
				break;
			}
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER ) {}

	LeaveCriticalSection( &csMenuHook );
	return res;
}

//wparam=0;
//lparam=PMenuParam;
//result=MenuObjectHandle
int MO_CreateNewMenuObject(WPARAM wParam,LPARAM lParam)
{
	PMenuParam pmp = ( PMenuParam )lParam;
	if ( !bIsGenMenuInited || pmp == NULL )
		return -1;

	EnterCriticalSection( &csMenuHook );
	TIntMenuObject* p = new TIntMenuObject();
	p->id = NextObjectId++;
	p->Name = mir_strdup( pmp->name );
	p->CheckService = mir_strdup( pmp->CheckService );
	p->ExecService = mir_strdup( pmp->ExecService );
	p->m_hMenuIcons = ImageList_Create( GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32|ILC_MASK, 15, 100 );
	g_menus.insert(p);

	LeaveCriticalSection( &csMenuHook );
	return p->id;
}

//wparam=MenuItemHandle
//lparam=0
int MO_RemoveMenuItem(WPARAM wParam,LPARAM lParam)
{
	int objidx,menuitemidx;//pos in array
	ListParam lp;
	int res;

	EnterCriticalSection( &csMenuHook );
	if ( GetAllIdx( wParam, &objidx, &menuitemidx ) == 0 ) {
		LeaveCriticalSection( &csMenuHook );
		return 0;
	}

	lp.MenuObjectHandle = g_menus[objidx]->id;
	res = RecursiveRemoveChilds( menuitemidx, &lp );
	LeaveCriticalSection( &csMenuHook );
	return res;
}


static int GetNextObjectMenuItemId()
{
	int menuID=CLISTMENUIDMIN;
	if (NextObjectMenuItemId>CLISTMENUIDMAX/2)
	{
		// TODO: try to reuse not used (removed menus) ids
		menuID=NextObjectMenuItemId++;
	}
	else 
	{
		// TODO: otherwise simple increase
		menuID=NextObjectMenuItemId++;
	}

	if (menuID>CLISTMENUIDMAX) 
	{	
		MessageBox(NULL,TranslateT("Too many menu items registered, please restart your Miranda to avoid unpredictable behaviour"),
			TranslateT("Error"),
			MB_OK|MB_ICONERROR);
		NextObjectMenuItemId=CLISTMENUIDMIN;
	}
	return menuID;
}

//wparam=MenuObjectHandle
//lparam=PMO_MenuItem
//return MenuItemHandle
int MO_AddNewMenuItem( int menuobjecthandle, PMO_MenuItem pmi )
{
	if ( !bIsGenMenuInited || pmi == NULL )
		return -1;

	EnterCriticalSection( &csMenuHook );
	int objidx = GetMenuObjbyId( menuobjecthandle );
	if ( objidx == -1 || pmi->cbSize != sizeof( TMO_MenuItem )) {
		LeaveCriticalSection( &csMenuHook );
		return -1;
	}

	//old mode
	if ( !( pmi->flags & CMIF_ROOTPOPUP || pmi->flags & CMIF_CHILDPOPUP )) {
		LeaveCriticalSection( &csMenuHook );
		return MO_AddOldNewMenuItem( menuobjecthandle, pmi );
	}

	TIntMenuObject* pmo = g_menus[objidx];

	TMO_IntMenuItem* p = ( TMO_IntMenuItem* )mir_calloc( sizeof( TMO_IntMenuItem ));
	pmo->m_items.insert( p );

	p->id = GetNextObjectMenuItemId();
	p->globalid = getGlobalId( menuobjecthandle, p->id );
	p->mi = *pmi;
	p->iconId = -1;
	p->OverrideShow = TRUE;
	p->originalPosition = pmi->position;
	#if defined( _UNICODE )
		if ( pmi->flags & CMIF_UNICODE ) 
			p->mi.ptszName = mir_tstrdup(( pmi->flags & CMIF_KEEPUNTRANSLATED ) ? pmi->ptszName : TranslateTS( pmi->ptszName ));
		else {
			if ( pmi->flags & CMIF_KEEPUNTRANSLATED ) {
				int len = lstrlenA( pmi->pszName );
				p->mi.ptszName = ( TCHAR* )mir_alloc( sizeof( TCHAR )*( len+1 ));
				MultiByteToWideChar( CP_ACP, 0, pmi->pszName, -1, p->mi.ptszName, len+1 );
				p->mi.ptszName[ len ] = 0;
			}
			else p->mi.ptszName = LangPackPcharToTchar( pmi->pszName );
		}
	#else
		p->mi.ptszName = mir_strdup(( pmi->flags & CMIF_KEEPUNTRANSLATED ) ? pmi->ptszName : Translate( pmi->ptszName ));
	#endif

	if ( pmi->hIcon != NULL ) {
		if ( pmi->flags & CMIF_ICONFROMICOLIB ) {
			HICON hIcon = IcoLib_GetIconByHandle( pmi->hIcolibItem );
			p->iconId = ImageList_AddIcon( pmo->m_hMenuIcons, hIcon );
			p->hIcolibItem = pmi->hIcolibItem;
			IconLib_ReleaseIcon( hIcon, 0 );
		}
		else {
			HANDLE hIcolibItem = IcoLib_IsManaged( pmi->hIcon );
			if ( hIcolibItem ) {
				p->iconId = ImageList_AddIcon( pmo->m_hMenuIcons, pmi->hIcon );
				p->hIcolibItem = hIcolibItem;
			}
			else p->iconId = ImageList_AddIcon( pmo->m_hMenuIcons, pmi->hIcon );
	}	}

	LeaveCriticalSection( &csMenuHook );
	return p->globalid;
}

//wparam=MenuObjectHandle
//lparam=PMO_MenuItem

int MO_AddOldNewMenuItem( int menuobjecthandle, PMO_MenuItem pmi )
{
	int objidx;
	int oldroot,i;

	if ( !bIsGenMenuInited || pmi == NULL )
		return -1;

	objidx = GetMenuObjbyId( menuobjecthandle );
	if ( objidx == -1 )
		return -1;

	if ( pmi->cbSize != sizeof( TMO_MenuItem ))
		return 0;

	if (( pmi->flags & CMIF_ROOTPOPUP ) || ( pmi->flags & CMIF_CHILDPOPUP ))
		return 0;

	//is item with popup or not
	if ( pmi->root == 0 ) {
		//yes,this without popup
		pmi->root = -1; //first level
	}
	else { // no,search for needed root and create it if need
		TCHAR* tszRoot;
#if defined( _UNICODE )
		if ( pmi->flags & CMIF_UNICODE )
			tszRoot = mir_tstrdup(( TCHAR* )pmi->root );
		else
			tszRoot = LangPackPcharToTchar(( char* )pmi->root );
#else
		tszRoot = mir_tstrdup(( TCHAR* )pmi->root );
#endif

		oldroot = -1;
		for ( i=0; i < g_menus[objidx]->m_items.getCount(); i++ ) {
			TMO_IntMenuItem* p = g_menus[objidx]->m_items[i];
			if ( p->mi.pszName == NULL )
				continue;
			if (( p->mi.flags & CMIF_ROOTPOPUP ) && !_tcscmp( p->mi.ptszName, tszRoot)) {
				oldroot = getGlobalId( g_menus[objidx]->id, p->id );
				break;
			}	}
		mir_free( tszRoot );

		if ( oldroot == -1 ) {
			//not found,creating root
			TMO_MenuItem tmi = { 0 };
			tmi = *pmi;
			tmi.flags |= CMIF_ROOTPOPUP;
			tmi.ownerdata = 0;
			tmi.root = -1;
			//copy pszPopupName
			tmi.ptszName = ( TCHAR* )pmi->root;
			if (( oldroot = MO_AddNewMenuItem( menuobjecthandle, &tmi )) != -1 )
				MO_SetOptionsMenuItem( oldroot, OPT_MENUITEMSETUNIQNAME, (int)pmi->root );
		}
		pmi->root = oldroot;
		//popup will be created in next commands
	}
	pmi->flags |= CMIF_CHILDPOPUP;
	//add popup(root allready exists)
	return MO_AddNewMenuItem( menuobjecthandle, pmi );
}

static int WhereToPlace(HMENU hMenu,PMO_MenuItem mi,MENUITEMINFO *mii,ListParam *param)
{
	int i=0;

	mii->fMask = MIIM_SUBMENU | MIIM_DATA;
	for ( i=GetMenuItemCount( hMenu )-1; i >= 0; i-- ) {
		GetMenuItemInfo( hMenu, i, TRUE, mii );
		if ( mii->fType != MFT_SEPARATOR ) {
			PMO_IntMenuItem pimi = MO_GetIntMenuItem(mii->dwItemData);
			if ( pimi != NULL )
				if ( pimi->mi.position <= mi->position )
					break;
		}	}

	return i+1;
}

static void InsertMenuItemWithSeparators(HMENU hMenu,int uItem,BOOL fByPosition,MENUITEMINFO *lpmii,ListParam *param)
{
	int thisItemPosition, needSeparator = 0, itemidx;
	MENUITEMINFO mii;
	PMO_IntMenuItem items=NULL;
	PMO_IntMenuItem pimi=NULL;

	int objid,menuitemid;//id
	int objidx,menuitemidx;//pos in array

	UnpackGlobalId( lpmii->dwItemData, &objid, &menuitemid );
	if ( objid == -1 || menuitemid == -1 )
		return;

	objidx = GetMenuObjbyId( objid );
	menuitemidx = GetMenuItembyId( objidx, menuitemid );
	if ( objidx == -1 || menuitemidx == -1 )
		return;

	itemidx = menuitemidx;
	thisItemPosition = g_menus[objidx]->m_items[itemidx]->mi.position;

	ZeroMemory( &mii, sizeof( mii ));
	mii.cbSize = MENUITEMINFO_V4_SIZE;
	//check for separator before
	if ( uItem ) {
		mii.fMask = MIIM_SUBMENU | MIIM_DATA | MIIM_TYPE;
		GetMenuItemInfo( hMenu, uItem-1, TRUE, &mii );
		pimi = MO_GetIntMenuItem( mii.dwItemData );
		if ( pimi != NULL ) {
			if ( mii.fType == MFT_SEPARATOR )
				needSeparator = 0;
			else
				needSeparator = ( pimi->mi.position / SEPARATORPOSITIONINTERVAL ) != thisItemPosition / SEPARATORPOSITIONINTERVAL;
		}
		if ( needSeparator) {
			//but might be supposed to be after the next one instead
			mii.fType = 0;
			if ( uItem < GetMenuItemCount( hMenu )) {
				mii.fMask = MIIM_SUBMENU | MIIM_DATA | MIIM_TYPE;
				GetMenuItemInfo( hMenu, uItem, TRUE, &mii );
			}
			if ( mii.fType != MFT_SEPARATOR) {
				mii.fMask = MIIM_TYPE;
				mii.fType = MFT_SEPARATOR;
				InsertMenuItem( hMenu, uItem, TRUE, &mii );
			}
			uItem++;
		}	}

	//check for separator after
	if ( uItem < GetMenuItemCount( hMenu )) {
		mii.fMask = MIIM_SUBMENU | MIIM_DATA | MIIM_TYPE;
		mii.cch = 0;
		GetMenuItemInfo( hMenu, uItem, TRUE, &mii );
		pimi = MO_GetIntMenuItem( mii.dwItemData );
		if ( pimi != NULL ) {
			if ( mii.fType == MFT_SEPARATOR )
				needSeparator=0;
			else
				needSeparator = pimi->mi.position / SEPARATORPOSITIONINTERVAL != thisItemPosition / SEPARATORPOSITIONINTERVAL;
		}
		if ( needSeparator) {
			mii.fMask = MIIM_TYPE;
			mii.fType = MFT_SEPARATOR;
			InsertMenuItem( hMenu, uItem, TRUE, &mii );
		}	}

	if ( uItem == GetMenuItemCount( hMenu )-1 ) {
		TCHAR str[32];
		mii.fMask = MIIM_SUBMENU | MIIM_DATA | MIIM_TYPE;
		mii.dwTypeData = str;
		mii.cch = SIZEOF( str );
		GetMenuItemInfo( hMenu, uItem, TRUE, &mii );
	}

	{	// create local copy *lpmii so we can change some flags
		MENUITEMINFO mii_copy = *lpmii;
		lpmii = &mii_copy;

		if (( GetMenuItemCount( hMenu ) % 35 ) == 33 /* will be 34 after addition :) */ && pimi != NULL )
			if ( pimi->mi.root != -1 )
			{
				if (!(lpmii->fMask&MIIM_FTYPE))
					lpmii->fType = 0;
				lpmii->fMask |= MIIM_FTYPE;
				lpmii->fType |= MFT_MENUBARBREAK;
			}

			InsertMenuItem( hMenu, uItem, TRUE, lpmii );
	}
}

//wparam started hMenu
//lparam ListParam*
//result hMenu
int MO_BuildMenu(WPARAM wParam,LPARAM lParam)
{
	if ( !bIsGenMenuInited )
		return -1;

	EnterCriticalSection( &csMenuHook );

	ListParam *lp = ( ListParam* )lParam;
	int pimoidx = GetMenuObjbyId( lp->MenuObjectHandle );
	if ( pimoidx == -1 ) {
		LeaveCriticalSection( &csMenuHook );
		return 0;
	}

	int res = (int)BuildRecursiveMenu(( HMENU )wParam, ( ListParam* )lParam );
	LeaveCriticalSection( &csMenuHook );
	return res;
}

#ifdef _DEBUG
#define PUTPOSITIONSONMENU
#endif

void GetMenuItemName( PMO_IntMenuItem pMenuItem, char* pszDest, size_t cbDestSize )
{
	if ( pMenuItem->UniqName )
		mir_snprintf( pszDest, cbDestSize, "{%s}", pMenuItem->UniqName );
	else {
		#if defined( _UNICODE )
			char* name = u2a( pMenuItem->mi.ptszName );
			mir_snprintf( pszDest, cbDestSize, "{%s}", name );
			mir_free(name);
		#else
			mir_snprintf( pszDest, cbDestSize, "{%s}", pMenuItem->mi.pszName );
		#endif
}	}

HMENU BuildRecursiveMenu(HMENU hMenu,ListParam *param)
{
	if ( param == NULL )
		return NULL;

	int pimoidx = GetMenuObjbyId( param->MenuObjectHandle );
	if ( pimoidx == -1 )
		return NULL;

	int itemsCount = g_menus[pimoidx]->m_items.getCount();
	int rootlevel = param->rootlevel;
	char* checkproc = g_menus[pimoidx]->CheckService;
	char* onAddproc = g_menus[pimoidx]->onAddService;

	ListParam localparam = *param;

	char MenuNameItems[256];
	wsprintfA(MenuNameItems, "%s_Items", g_menus[pimoidx]->Name);

	BOOL bIsConversionDone = LangPackGetDefaultCodePage() == CP_ACP; // check the langpack presence
	if ( !bIsConversionDone )
		bIsConversionDone = DBGetContactSettingByte( NULL, MenuNameItems, "LangpackConversion", 0 );

	while ( rootlevel == -1 && GetMenuItemCount( hMenu ) > 0 )
		DeleteMenu( hMenu, 0, MF_BYPOSITION );

	for ( int j=0; j < itemsCount; j++ ) {
		PMO_IntMenuItem pmi = g_menus[pimoidx]->m_items[j];
		PMO_MenuItem mi = &pmi->mi;
		if ( mi->cbSize != sizeof( TMO_MenuItem ))
			continue;

		if ( checkproc != NULL )	{
			TCheckProcParam CheckParam;
			CheckParam.lParam = param->lParam;
			CheckParam.wParam = param->wParam;
			CheckParam.MenuItemOwnerData = mi->ownerdata;
			CheckParam.MenuItemHandle = getGlobalId( param->MenuObjectHandle, pmi->id );
			if ( CallService( checkproc, ( WPARAM )&CheckParam, 0 ) == FALSE )
				continue;
		}

		/**************************************/
		if ( rootlevel == -1 && mi->root == -1 && g_menus[pimoidx]->m_bUseUserDefinedItems ) {
			char menuItemName[256], menuItemNameLP[256];
			char DBString[256], DBStringLP[256];
			DBVARIANT dbv;
			int pos;
			BOOL bCheckDouble = FALSE;
			memset(&dbv,0,sizeof(dbv));

			GetMenuItemName( pmi, menuItemName, sizeof( menuItemName ));
			if ( !bIsConversionDone ) {
				strcpy( DBString, menuItemName+1 );
				DBString[ strlen(DBString)-1 ] = 0;
				mir_snprintf( menuItemNameLP, sizeof(menuItemNameLP), "{%s}", Translate( DBString ));
				bCheckDouble = strcmp( menuItemName, menuItemNameLP ) != 0;
			}

			// check if it visible
			wsprintfA( DBString, "%s_visible", menuItemName );
			if ( bCheckDouble ) {
				wsprintfA( DBStringLP, "%s_visible", menuItemNameLP );
				if ( !DBGetContactSetting( NULL, MenuNameItems, DBStringLP, &dbv )) {
					DBDeleteContactSetting( NULL, MenuNameItems, DBStringLP );
					DBWriteContactSettingByte( NULL, MenuNameItems, DBString, dbv.bVal );
			}	}

			if ( DBGetContactSettingByte( NULL, MenuNameItems, DBString, -1 ) == -1 )
				DBWriteContactSettingByte( NULL, MenuNameItems, DBString, 1 );

			pmi->OverrideShow = TRUE;
			if ( !DBGetContactSettingByte( NULL, MenuNameItems, DBString, 1 )) {
				pmi->OverrideShow = FALSE;
				continue;  // find out what value to return if not getting added
			}

			// mi.pszName
			wsprintfA( DBString, "%s_name", menuItemName );
			if ( bCheckDouble ) {
				wsprintfA( DBStringLP, "%s_name", menuItemNameLP );
				if ( !DBGetContactSettingTString( NULL, MenuNameItems, DBStringLP, &dbv )) {
					DBDeleteContactSetting( NULL, MenuNameItems, DBStringLP );
					DBWriteContactSettingTString( NULL, MenuNameItems, DBString, dbv.ptszVal );
			}	}

			if ( !DBGetContactSettingTString( NULL, MenuNameItems, DBString, &dbv )) {
				if ( _tcslen( dbv.ptszVal ) > 0 ) {
					if ( pmi->CustomName ) mir_free( pmi->CustomName );
					pmi->CustomName = mir_tstrdup( dbv.ptszVal );
				}
				DBFreeVariant( &dbv );
			}

			wsprintfA( DBString, "%s_pos", menuItemName );
			if ( bCheckDouble ) {
				wsprintfA( DBStringLP, "%s_pos", menuItemNameLP );
				if ( !DBGetContactSetting( NULL, MenuNameItems, DBStringLP, &dbv )) {
					DBDeleteContactSetting( NULL, MenuNameItems, DBStringLP );
					DBWriteContactSettingDword( NULL, MenuNameItems, DBString, dbv.lVal );
			}	}

			if (( pos = DBGetContactSettingDword( NULL, MenuNameItems, DBString, -1 )) == -1 ) {
				DBWriteContactSettingDword( NULL, MenuNameItems, DBString, mi->position );
				if ( mi->flags & CMIF_ROOTPOPUP )
					mi->position = 0;
			}
			else mi->position = pos;
		}

		if ( !bIsConversionDone )
			DBWriteContactSettingByte( NULL, MenuNameItems, "LangpackConversion", 1 );

		/**************************************/

		MENUITEMINFO mii = { 0 };
		int i = 0;

		//hMenu=hMainMenu;
		mii.cbSize = MENUITEMINFO_V4_SIZE;
		mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_DATA;
		mii.dwItemData = getGlobalId( param->MenuObjectHandle, pmi->id );
		HMENU hSubMenu = NULL;

		if ( rootlevel != (int)pmi->mi.root )
			continue;

		if ( pmi->mi.flags & ( CMIF_ROOTPOPUP | CMIF_CHILDPOPUP )) {
			if ( rootlevel == (int)pmi->mi.root ) {
				//our level
				if ( pmi->mi.flags & CMIF_ROOTPOPUP ) {
					i = WhereToPlace( hMenu, mi, &mii, &localparam );

					if ( !IsWinVer98Plus()) {
						mii.cbSize = MENUITEMINFO_V4_SIZE;
						mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;
					}
					else {
						mii.cbSize = sizeof( mii );
						mii.fMask = MIIM_DATA | MIIM_ID | MIIM_STRING | MIIM_SUBMENU;
						if ( pmi->iconId != -1 )
							mii.fMask |= MIIM_BITMAP;
					}

					mii.fType = MFT_STRING;
					mii.dwItemData = pmi->globalid;

					hSubMenu = CreatePopupMenu();
					mii.hSubMenu = hSubMenu;
					mii.hbmpItem = HBMMENU_CALLBACK;
					mii.dwTypeData = ( pmi->CustomName ) ? pmi->CustomName : mi->ptszName;
					pmi->hSubMenu = hSubMenu;

					#ifdef PUTPOSITIONSONMENU
						if ( GetKeyState(VK_CONTROL) & 0x8000) {
							TCHAR str[256];
							mir_sntprintf( str, SIZEOF(str), _T( "%s (%d,id %x)" ), mi->pszName, mi->position, mii.dwItemData );
							mii.dwTypeData = str;
						}
					#endif

					InsertMenuItemWithSeparators( hMenu, i, TRUE, &mii, &localparam );
					localparam.rootlevel = getGlobalId( param->MenuObjectHandle, pmi->id );
					BuildRecursiveMenu( hSubMenu, &localparam );
					continue;
				}

				i = WhereToPlace( hMenu, mi, &mii, &localparam );

				if ( !IsWinVer98Plus()) {
					mii.cbSize = MENUITEMINFO_V4_SIZE;
					mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID | MIIM_STATE;
				}
				else {
					mii.cbSize = sizeof( mii );
					mii.fMask = MIIM_DATA | MIIM_ID | MIIM_STRING | MIIM_STATE;
					if ( pmi->iconId != -1 )
						mii.fMask |= MIIM_BITMAP;
				}
				mii.fState = (( pmi->mi.flags & CMIF_GRAYED ) ? MFS_GRAYED : MFS_ENABLED );
				mii.fState |= (( pmi->mi.flags & CMIF_CHECKED) ? MFS_CHECKED : MFS_UNCHECKED );
				mii.fType = MFT_STRING;
				mii.wID = pmi->id;
				mii.dwItemData = pmi->globalid;

				mii.hbmpItem = HBMMENU_CALLBACK;
				mii.dwTypeData = ( pmi->CustomName ) ? pmi->CustomName : mi->ptszName;

				#ifdef PUTPOSITIONSONMENU
					if ( GetKeyState(VK_CONTROL) & 0x8000) {
						TCHAR str[256];
						mir_sntprintf( str, SIZEOF(str), _T("%s (%d,id %x)"), mi->pszName, mi->position, mii.dwItemData );
						mii.dwTypeData = str;
					}
				#endif

				if ( onAddproc != NULL )
					if ( CallService( onAddproc, ( WPARAM )&mii, ( LPARAM )pmi->globalid ) == FALSE )
						continue;

				InsertMenuItemWithSeparators( hMenu, i, TRUE, &mii, &localparam );
				continue;
	}	}	}

	return hMenu;
}

int RecursiveRemoveChilds(int pos,ListParam *param)
{
	int objidx = GetMenuObjbyId( param->MenuObjectHandle );
	if ( objidx == -1 )
		return -1;

	TIntMenuObject* pmo = g_menus[objidx];

	ListParam localparam = *param;

	int rootid = getGlobalId( pmo->id, pmo->m_items[pos]->id );
	int menuitemid = pmo->m_items[pos]->id;

	int i=0;
	if ( pmo->m_items[pos]->mi.flags & CMIF_ROOTPOPUP ) {
		while( i < pmo->m_items.getCount() ) {
			if (pmo->m_items[i]->mi.root != rootid || pmo->m_items[i]->mi.root == -1 ) {
				i++;
				continue;
			}
			if ( pmo->m_items[i]->mi.flags & CMIF_ROOTPOPUP ) {
				RecursiveRemoveChilds( i, &localparam );
				i = 0;
				continue;
			}
			if ( pmo->m_items[i]->mi.flags & CMIF_CHILDPOPUP ) {
				pmo->freeItem( pmo->m_items[i] );
				pmo->m_items.remove( i );
				continue;
	}	}	}

	i = GetMenuItembyId( objidx, menuitemid );
	if ( i >= 0 && i < pmo->m_items.getCount() ) {
		pmo->freeItem( pmo->m_items[i] );
		pmo->m_items.remove( i );
	}
	return 0;
}

/* iconlib in menu */
extern int hStatusMenuObject;
HICON LoadIconFromLibrary(char *SectName,char *Name,char *Description,HICON hIcon,boolean RegisterIt,boolean *RegistredOk)
{
	return hIcon;
}

int OnIconLibChanges(WPARAM wParam,LPARAM lParam)
{
	EnterCriticalSection( &csMenuHook );
	for ( int mo=0; mo < g_menus.getCount(); mo++ ) {
		if ( hStatusMenuObject == g_menus[mo]->id ) //skip status menu
			continue;

		for ( int mi=0; mi < g_menus[mo]->m_items.getCount(); mi++ ) {
			PMO_IntMenuItem pmi = g_menus[mo]->m_items[mi];
			if ( pmi->hIcolibItem ) {
				HICON newIcon = IcoLib_GetIconByHandle( pmi->hIcolibItem );
				if ( newIcon )
					ImageList_ReplaceIcon( g_menus[mo]->m_hMenuIcons, pmi->iconId, newIcon );

				IconLib_ReleaseIcon(newIcon,0);
	}	}	}

	LeaveCriticalSection( &csMenuHook );
	return 0;
}

int RegisterAllIconsInIconLib()
{
	//register all icons
	for ( int mo=0; mo < g_menus.getCount(); mo++ ) {
		if ( hStatusMenuObject == g_menus[mo]->id ) //skip status menu
			continue;

		for ( int mi=0; mi < g_menus[mo]->m_items.getCount(); mi++ ) {
			PMO_IntMenuItem pmi = g_menus[mo]->m_items[mi];
			char *uname, *descr;

			uname = pmi->UniqName;
			if ( uname == NULL )
				#ifdef UNICODE
					uname = u2a(pmi->CustomName);
					descr = u2a(pmi->mi.ptszName);
				#else
					uname = pmi->CustomName;
					descr = pmi->mi.pszName;
				#endif

			if ( !uname && !descr )
				continue;

			if ( !pmi->hIcolibItem ) {
				HICON hIcon = ImageList_GetIcon( g_menus[mo]->m_hMenuIcons, pmi->iconId, 0 );
				char* buf = NEWSTR_ALLOCA( descr );

				char sectionName[256], iconame[256];
				_snprintf( sectionName, sizeof(sectionName), "Menu Icons/%s", g_menus[mo]->Name );

				// remove '&'
				char* start = buf;
				while ( start = strchr( start, '&' )) {
					memmove(start,start+1,strlen(start+1)+1);
					if (*start!='\0') start++;
					else break;
				}

				if ( uname != NULL && *uname != 0 )
					_snprintf(iconame,sizeof(iconame),"genmenu_%s_%s", g_menus[mo]->Name, uname );
				else
					_snprintf(iconame,sizeof(iconame),"genmenu_%s_%s", g_menus[mo]->Name, descr );

				SKINICONDESC sid={0};
				sid.cbSize = sizeof(sid);
				sid.cx = 16;
				sid.cy = 16;
				sid.pszSection = Translate(sectionName);
				sid.pszName = iconame;
				sid.pszDefaultFile = NULL;
				sid.pszDescription = buf;
				sid.hDefaultIcon = hIcon;
				pmi->hIcolibItem = ( HANDLE )CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

				Safe_DestroyIcon( hIcon );
				if ( hIcon = ( HICON )CallService( MS_SKIN2_GETICON, 0, (LPARAM)iconame )) {
					ImageList_ReplaceIcon( g_menus[mo]->m_hMenuIcons, pmi->iconId, hIcon );
					IconLib_ReleaseIcon( hIcon, 0 );
			}	}

			#ifdef UNICODE
				if ( !pmi->UniqName )
					mir_free( uname );
				mir_free( descr );
			#endif
	}	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Static services

int posttimerid;

static VOID CALLBACK PostRegisterIcons( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	KillTimer( 0, posttimerid );
	RegisterAllIconsInIconLib();
}

static int OnModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	posttimerid = SetTimer(( HWND )NULL, 0, 5, ( TIMERPROC )PostRegisterIcons );
	HookEvent(ME_SKIN2_ICONSCHANGED,OnIconLibChanges);
	return 0;
}

static int SRVMO_SetOptionsMenuObject( WPARAM wParam, LPARAM lParam)
{
	lpOptParam lpop = ( lpOptParam )lParam;
	if ( lpop == NULL )
		return 0;

	return MO_SetOptionsMenuObject( lpop->Handle, lpop->Setting, lpop->Value );
}

static int SRVMO_SetOptionsMenuItem( WPARAM wParam, LPARAM lParam)
{
	lpOptParam lpop = ( lpOptParam )lParam;
	if ( lpop == NULL )
		return 0;

	return MO_SetOptionsMenuItem( lpop->Handle, lpop->Setting, lpop->Value );
}

int InitGenMenu()
{
	InitializeCriticalSection( &csMenuHook );
	CreateServiceFunction( MO_BUILDMENU, MO_BuildMenu );

	CreateServiceFunction( MO_PROCESSCOMMAND, MO_ProcessCommand );
	CreateServiceFunction( MO_CREATENEWMENUOBJECT, MO_CreateNewMenuObject );
	CreateServiceFunction( MO_REMOVEMENUITEM, MO_RemoveMenuItem );
	CreateServiceFunction( MO_ADDNEWMENUITEM, ( MIRANDASERVICE )MO_AddNewMenuItem );
	CreateServiceFunction( MO_MENUITEMGETOWNERDATA, MO_MenuItemGetOwnerData );
	CreateServiceFunction( MO_MODIFYMENUITEM, ( MIRANDASERVICE )MO_ModifyMenuItem );
	CreateServiceFunction( MO_GETMENUITEM, MO_GetMenuItem );
	CreateServiceFunction( MO_PROCESSCOMMANDBYMENUIDENT, MO_ProcessCommandByMenuIdent );
	CreateServiceFunction( MO_PROCESSHOTKEYS, ( MIRANDASERVICE )MO_ProcessHotKeys );
	CreateServiceFunction( MO_REMOVEMENUOBJECT, MO_RemoveMenuObject );

	CreateServiceFunction( MO_SETOPTIONSMENUOBJECT, SRVMO_SetOptionsMenuObject );
	CreateServiceFunction( MO_SETOPTIONSMENUITEM, SRVMO_SetOptionsMenuItem );

	EnterCriticalSection( &csMenuHook );
	bIsGenMenuInited = TRUE;
	LeaveCriticalSection( &csMenuHook );

	HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded );
	HookEvent( ME_OPT_INITIALISE,       GenMenuOptInit  );
	return 0;
}

int UnitGenMenu()
{
	if ( bIsGenMenuInited ) {
		EnterCriticalSection( &csMenuHook );
		MO_RemoveAllObjects();
		bIsGenMenuInited=FALSE;

		LeaveCriticalSection( &csMenuHook );
		DeleteCriticalSection(&csMenuHook);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

TIntMenuObject::TIntMenuObject() :
	m_items( 100, NULL )	
{
}

TIntMenuObject::~TIntMenuObject()
{
	for ( int j=0; j < m_items.getCount(); j++ )
		freeItem( m_items[j] );

	m_items.destroy();

	FreeAndNil(( void** )&FreeService );
	FreeAndNil(( void** )&onAddService );
	FreeAndNil(( void** )&CheckService );
	FreeAndNil(( void** )&ExecService );
	FreeAndNil(( void** )&Name );

	ImageList_Destroy(m_hMenuIcons);
}

void TIntMenuObject::freeItem( TMO_IntMenuItem* p )
{
	if ( FreeService )
		CallService( FreeService, ( WPARAM )p->globalid, ( LPARAM )p->mi.ownerdata );

	FreeAndNil(( void** )&p->mi.pszName );
	FreeAndNil(( void** )&p->UniqName   );
	FreeAndNil(( void** )&p->CustomName );
	mir_free( p );
}
