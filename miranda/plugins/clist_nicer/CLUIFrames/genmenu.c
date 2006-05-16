#include "../commonheaders.h"
#include "../m_genmenu.h"
#include "genmenu.h"

#pragma hdrstop

//menu object array
PIntMenuObject MenuObjects=NULL;
int MenuObjectsCount=0;
int NextObjectId=0x100;
int NextObjectMenuItemId=0x37;
boolean isGenMenuInited=FALSE;

static CRITICAL_SECTION csMenuHook;


static void __inline lockmo()
{
	EnterCriticalSection(&csMenuHook);
}

static void __inline unlockmo()
{
	LeaveCriticalSection(&csMenuHook);
}

void FreeAndNil(void **p)
{
	if (p!=NULL)
	{
		if (*p!=NULL)
		{
			if (!IsBadCodePtr(*p))
			{
				mir_free(*p);
			}
			*p=NULL;
		}
	}
}

int GetMenuObjbyId(const int id)
{
	int i;
	for (i=0;i<MenuObjectsCount;i++)
	{
		if (MenuObjects[i].id==id)
		{
			return(i);
		}
	}
	return(-1);
}

int GetMenuItembyId(const int objpos,const int id)
{
	int i;
	if ( (objpos<0)||(objpos>=MenuObjectsCount)){return(-1);}
	for (i=0;i<MenuObjects[objpos].MenuItemsCount;i++)
	{
		if (MenuObjects[objpos].MenuItems[i].id==id)
		{
			return(i);
		}
	}
	return(-1);
}
int getGlobalId(const int MenuObjectId,const int MenuItemId)
{
	return((MenuObjectId <<16)+MenuItemId);
}

int GetObjIdxByItemId(const int MenuItemId)
{
	int i,j;
	for (i=0;i<MenuObjectsCount;i++)
	{
		for (j=0;j<MenuObjects[i].MenuItemsCount;j++)
		{
			if (MenuObjects[i].MenuItems[j].id==MenuItemId)
			{
				return(i);
			}

		}

	}
	return(-1);
}
//return 0-erorr,1-ok
int GetAllIdx(const int globid,int *pobjidx,int *pmenuitemidx)
{
	int objid,menuitemid;//id
	int objidx,menuitemidx;//pos in array

	if (pobjidx==NULL||pmenuitemidx==NULL){return(0);}

	UnpackGlobalId(globid,&objid,&menuitemid);
	if ((objid==-1)||(menuitemid==-1)){return(0);}
	objidx=GetMenuObjbyId(objid);
	menuitemidx=GetMenuItembyId(objidx,menuitemid);
	if ((objidx==-1)||(menuitemidx==-1)){return(0);}
	*pobjidx=objidx;
	*pmenuitemidx=menuitemidx;
	return(1);
}

void UnpackGlobalId(int id,int *MenuObjectId,int *MenuItemId)
{
	if (MenuObjectId==NULL){return;}
	if (MenuItemId==NULL){return;}
	*MenuObjectId=-1;
	*MenuItemId=-1;
	*MenuObjectId=(id);
	*MenuObjectId=(*MenuObjectId>>16);

	*MenuItemId=id&0x0000FFFF;
}

//wparam=0
//lparam=LPMEASUREITEMSTRUCT
static int MO_MeasureMenuItem(WPARAM wParam,LPARAM lParam)
{
	PMO_IntMenuItem pimi=NULL;
	LPMEASUREITEMSTRUCT mis=(LPMEASUREITEMSTRUCT)lParam;

	if (!isGenMenuInited) return -1;

	if (mis==NULL){return(FALSE);}
	pimi=MO_GetIntMenuItem(mis->itemData);
	if (pimi==NULL) return FALSE;
	if (pimi->iconId==-1) return FALSE;

	mis->itemWidth=max(0,GetSystemMetrics(SM_CXSMICON)-GetSystemMetrics(SM_CXMENUCHECK)+4);
	mis->itemHeight=GetSystemMetrics(SM_CYSMICON)+2;
	return TRUE;
}

//wparam=0
//lparam=LPDRAWITEMSTRUCT
int MO_DrawMenuItem(WPARAM wParam,LPARAM lParam)
{
	PMO_IntMenuItem pimi=NULL;
	int y,objidx,menuitemidx;
	LPDRAWITEMSTRUCT dis=(LPDRAWITEMSTRUCT)lParam;

	if (!isGenMenuInited) return -1;

	if (dis==NULL){return(FALSE);}

	lockmo();

	if (GetAllIdx(dis->itemData,&objidx,&menuitemidx)==0){unlockmo();return(FALSE);}
	pimi=&MenuObjects[objidx].MenuItems[menuitemidx];

	if (pimi==NULL||pimi->iconId==-1) {unlockmo();return (FALSE);}

	y=(dis->rcItem.bottom-dis->rcItem.top-GetSystemMetrics(SM_CYSMICON))/2+1;
	if (dis->itemState&ODS_SELECTED)	{
		if (dis->itemState&ODS_CHECKED) {
			RECT rc;
			rc.left=2; rc.right=GetSystemMetrics(SM_CXSMICON)+2;
			rc.top=y; rc.bottom=rc.top+GetSystemMetrics(SM_CYSMICON)+2;
			FillRect(dis->hDC,&rc,GetSysColorBrush(COLOR_HIGHLIGHT));
			ImageList_DrawEx(MenuObjects[objidx].hMenuIcons,pimi->iconId,dis->hDC,2,y,0,0,CLR_NONE,CLR_DEFAULT,ILD_SELECTED);
		}
		else
			ImageList_DrawEx(MenuObjects[objidx].hMenuIcons,pimi->iconId,dis->hDC,2,y,0,0,CLR_NONE,CLR_DEFAULT,ILD_FOCUS);
		//DrawIconEx(dis->hDC,2,y,pimi->iconId,16,16,0,0,DI_NORMAL);
	}
	else {
		if (dis->itemState&ODS_CHECKED) {
			HBRUSH hBrush;
			RECT rc;
			COLORREF menuCol,hiliteCol;
			rc.left=0; rc.right=GetSystemMetrics(SM_CXSMICON)+4;
			rc.top=y-2; rc.bottom=rc.top+GetSystemMetrics(SM_CYSMICON)+4;
			DrawEdge(dis->hDC,&rc,BDR_SUNKENOUTER,BF_RECT);
			InflateRect(&rc,-1,-1);
			menuCol=GetSysColor(COLOR_MENU);
			hiliteCol=GetSysColor(COLOR_3DHIGHLIGHT);
			hBrush=CreateSolidBrush(RGB((GetRValue(menuCol)+GetRValue(hiliteCol))/2,(GetGValue(menuCol)+GetGValue(hiliteCol))/2,(GetBValue(menuCol)+GetBValue(hiliteCol))/2));
			FillRect(dis->hDC,&rc,GetSysColorBrush(COLOR_MENU));
			DeleteObject(hBrush);
			ImageList_DrawEx(MenuObjects[objidx].hMenuIcons,pimi->iconId,dis->hDC,2,y,0,0,CLR_NONE,GetSysColor(COLOR_MENU),ILD_BLEND50);
		}
		else
			ImageList_DrawEx(MenuObjects[objidx].hMenuIcons,pimi->iconId,dis->hDC,2,y,0,0,CLR_NONE,CLR_NONE,ILD_NORMAL);
		//DrawIconEx(dis->hDC,2,y,pimi->iconId,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,0,DI_NORMAL);
	}
	unlockmo();
	return TRUE;
}

void RemoveAndClearOneObject(int arpos)
{
	int j;
	for (j=0;j<MenuObjects[arpos].MenuItemsCount;j++) {
		if (MenuObjects[arpos].FreeService)
			CallService(MenuObjects[arpos].FreeService,(WPARAM)MenuObjects[arpos].MenuItems[j].globalid,(LPARAM)MenuObjects[arpos].MenuItems[j].mi.ownerdata);

		FreeAndNil(&MenuObjects[arpos].MenuItems[j].mi.pszName);
		FreeAndNil(&MenuObjects[arpos].MenuItems[j].UniqName);
		FreeAndNil(&MenuObjects[arpos].MenuItems[j].CustomName);
	}

	FreeAndNil(&MenuObjects[arpos].MenuItems);
	MenuObjects[arpos].MenuItemsCount=0;
	FreeAndNil(&MenuObjects[arpos].FreeService);
	FreeAndNil(&MenuObjects[arpos].onAddService);
	FreeAndNil(&MenuObjects[arpos].CheckService);
	FreeAndNil(&MenuObjects[arpos].ExecService);
	FreeAndNil(&MenuObjects[arpos].Name);


	ImageList_Destroy(MenuObjects[arpos].hMenuIcons);
}


int MO_RemoveAllObjects()
{
	int i;
	for (i=0;i<MenuObjectsCount;i++)
	{
		RemoveAndClearOneObject(i);
	}
	MenuObjectsCount=0;
	FreeAndNil(&MenuObjects);

	return(0);
}
//wparam=MenuObjectHandle
int MO_RemoveMenuObject(WPARAM wParam,LPARAM lParam)
{
	int objidx;

	if (!isGenMenuInited) return -1;
	lockmo();


	objidx=GetMenuObjbyId((int)wParam);
	if (objidx==-1){unlockmo();return(-1);}

	RemoveAndClearOneObject(objidx);

	RemoveFromList(objidx,&MenuObjects,&MenuObjectsCount,sizeof(MenuObjects[objidx]));
	unlockmo();
	return(0);
}


//wparam=MenuObjectHandle
//lparam=vKey
int MO_ProcessHotKeys(WPARAM wParam,LPARAM lParam)
{
	int objidx;//pos in array
	PMO_IntMenuItem pimi;
	int i;
	int vKey=(int)lParam;

	if (!isGenMenuInited) return -1;
	lockmo();
	objidx=GetMenuObjbyId((int)wParam);
	if (objidx==-1){unlockmo();return(FALSE);}


	for(i=0;i<MenuObjects[objidx].MenuItemsCount;i++) {
		pimi=&MenuObjects[objidx].MenuItems[i];
		if (pimi->mi.hotKey==0) continue;
		if (HIWORD(pimi->mi.hotKey)!=vKey) continue;
		if (!(LOWORD(pimi->mi.hotKey)&MOD_ALT)!=!(GetKeyState(VK_MENU)&0x8000)) continue;
		if (!(LOWORD(pimi->mi.hotKey)&MOD_CONTROL)!=!(GetKeyState(VK_CONTROL)&0x8000)) continue;
		if (!(LOWORD(pimi->mi.hotKey)&MOD_SHIFT)!=!(GetKeyState(VK_SHIFT)&0x8000)) continue;

		MO_ProcessCommand((WPARAM)getGlobalId(MenuObjects[objidx].id,pimi->id),(LPARAM)0);
		unlockmo();
		return TRUE;
	}

	unlockmo();
	return(FALSE);
}
//wparam=MenuItemHandle
//lparam=PMO_MenuItem
int MO_GetMenuItem(WPARAM wParam,LPARAM lParam)
{
	PMO_IntMenuItem pimi;
	PMO_MenuItem mi=(PMO_MenuItem)lParam;

	if (!isGenMenuInited) return -1;
	if (mi==NULL){return(-1);}
	lockmo();
	pimi=MO_GetIntMenuItem(wParam);
	if (pimi!=NULL){
		*mi=(pimi->mi);
		unlockmo();
		return(0);
	}
	unlockmo();
	return(-1);
}
//wparam MenuItemHandle
//lparam PMO_MenuItem
int MO_ModifyMenuItem(WPARAM wParam,LPARAM lParam)
{
	PMO_MenuItem pmiparam=(PMO_MenuItem)lParam;
	PMO_IntMenuItem pimi;
	int oldflags;
	int objidx;

	if (!isGenMenuInited) return -1;
	if (pmiparam==NULL){return(-1);}
	lockmo();
	pimi=MO_GetIntMenuItem(wParam);
	if (pimi==NULL){unlockmo();return(-1);}
	if (pmiparam->cbSize!=sizeof(TMO_MenuItem)){unlockmo();return(-1);}
	objidx=GetObjIdxByItemId(pimi->id);
	if (objidx==-1){unlockmo();return(-1);}

	if (pmiparam->flags&CMIM_NAME) {
		FreeAndNil(&pimi->mi.pszName);
		pimi->mi.pszName = mir_tstrdup(pmiparam->pszName);
	}
	if (pmiparam->flags&CMIM_FLAGS) {
		oldflags=(pimi->mi.flags&CMIF_ROOTPOPUP)|(pimi->mi.flags&CMIF_CHILDPOPUP);
		pimi->mi.flags=pmiparam->flags&~CMIM_ALL;
		pimi->mi.flags|=oldflags;
	}
	if (pmiparam->flags&CMIM_ICON) {
		pimi->mi.hIcon=pmiparam->hIcon;
		if (pmiparam->hIcon!=NULL) pimi->iconId=ImageList_ReplaceIcon(MenuObjects[objidx].hMenuIcons,pimi->iconId,pmiparam->hIcon);
		else pimi->iconId=-1;	  //fixme, should remove old icon & shuffle all iconIds
	}
	if (pmiparam->flags&CMIM_HOTKEY) {
		pimi->mi.hotKey=pmiparam->hotKey;
	}
	if (pmiparam->flags&CMIM_HOTKEY) {
		pimi->mi.hotKey=pmiparam->hotKey;
	}
	unlockmo();
	return(0);
}


//wparam MenuItemHandle
//return ownerdata useful to free ownerdata before delete menu item,
//NULL on error.
int MO_MenuItemGetOwnerData(WPARAM wParam,LPARAM lParam)
{
	int objid,menuitemid;//id
	int objidx,menuitemidx;//pos in array
	int res=0;

	if (!isGenMenuInited) return -1;
	lockmo();
	UnpackGlobalId(wParam,&objid,&menuitemid);
	if ((objid==-1)||(menuitemid==-1)){return(0);}
	objidx=GetMenuObjbyId(objid);
	menuitemidx=GetMenuItembyId(objidx,menuitemid);
	if ((objidx==-1)||(menuitemidx==-1)){return(0);}
	res=(int)MenuObjects[objidx].MenuItems[menuitemidx].mi.ownerdata;
	unlockmo();
	return(res);
}

PMO_IntMenuItem MO_GetIntMenuItem(int globid)
{
	int objid,menuitemid;//id
	int objidx,menuitemidx;//pos in array

	UnpackGlobalId(globid,&objid,&menuitemid);
	if ((objid==-1)||(menuitemid==-1)){return(0);}
	objidx=GetMenuObjbyId(objid);
	menuitemidx=GetMenuItembyId(objidx,menuitemid);
	if ((objidx==-1)||(menuitemidx==-1)){return(0);}
	return(&(MenuObjects[objidx].MenuItems[menuitemidx]));

}
//LOWORD(wparam) menuident
int MO_ProcessCommandByMenuIdent(WPARAM wParam,LPARAM lParam)
{
	int i,j;

	if (!isGenMenuInited) return -1;
	lockmo();
	for (i=0;i<MenuObjectsCount;i++)
	{
		for (j=0;j<MenuObjects[i].MenuItemsCount;j++)
		{
			if (MenuObjects[i].MenuItems[j].id==(int)wParam)
			{
				int globid;
				globid=getGlobalId(MenuObjects[i].id,MenuObjects[i].MenuItems[j].id);
				unlockmo();
				return(MO_ProcessCommand(globid,lParam));
			}
		}
	}
	unlockmo();
	return(FALSE);
}

int MO_ProcessCommand(WPARAM wParam,LPARAM lParam)
{

	int objidx,menuitemidx;//pos in array
	int globid=wParam;
	char *srvname;
	void *ownerdata;

	if (!isGenMenuInited) return -1;
	lockmo();
	if (GetAllIdx(globid,&objidx,&menuitemidx)==0){unlockmo();return(0);}
	srvname=MenuObjects[objidx].ExecService;
	ownerdata=MenuObjects[objidx].MenuItems[menuitemidx].mi.ownerdata;
	{
		char buf[256];
		sprintf(buf,"Process Command %s ",srvname);
	}
	unlockmo();
	CallService(srvname,(WPARAM)ownerdata,lParam);
	return(1);
}


int setcnt=0;
int MO_SetOptionsMenuItem(WPARAM wParam,LPARAM lParam)
{
	lpOptParam lpop;
	PMO_IntMenuItem pimi;
	int objidx;

	if (!isGenMenuInited) return -1;
	if (lParam==0){return(0);}
	lockmo();
	setcnt++;
	__try
	{
		lpop=(lpOptParam)lParam;
		pimi=MO_GetIntMenuItem(lpop->Handle);
		if (pimi==NULL){unlockmo();return(-1);}
		objidx=GetObjIdxByItemId(pimi->id);
		if (objidx==-1){unlockmo();return(-1);}

		switch(lpop->Setting) {
	 case OPT_MENUITEMSETUNIQNAME:
		 if (!(pimi->mi.flags&CMIF_ROOTPOPUP)) {
			 FreeAndNil(&pimi->UniqName);
			 pimi->UniqName=mir_strdup((char*)lpop->Value);
		 }
		 break;
		}
		return(1);
	}
	__finally
	{
		setcnt--;
		unlockmo();
	}
	return 1;
}

int MO_SetOptionsMenuObject(WPARAM wParam,LPARAM lParam)
{
	int  pimoidx;
	lpOptParam lpop;

	if (!isGenMenuInited) return -1;
	if (lParam==0){return(0);}
	lockmo();
	__try
	{
		lpop=(lpOptParam)lParam;
		pimoidx=GetMenuObjbyId(lpop->Handle);
		if (pimoidx==-1){return(0);}

		switch(lpop->Setting) {
		case OPT_MENUOBJECT_SET_ONADD_SERVICE:
			FreeAndNil(&MenuObjects[pimoidx].onAddService);
			MenuObjects[pimoidx].onAddService=mir_strdup((char *)lpop->Value);
			break;

		case OPT_MENUOBJECT_SET_FREE_SERVICE:
			FreeAndNil(&MenuObjects[pimoidx].FreeService);
			MenuObjects[pimoidx].FreeService=mir_strdup((char *)lpop->Value);
			break;

		case OPT_USERDEFINEDITEMS:
			MenuObjects[pimoidx].bUseUserDefinedItems=(BOOL)lpop->Value;
			break;
		}
		return(1);
	}
	__finally
	{
		unlockmo();
	}
	return 1;
}

//wparam=0;
//lparam=PMenuParam;
//result=MenuObjectHandle
int MO_CreateNewMenuObject(WPARAM wParam,LPARAM lParam)
{
	PMenuParam pmp=(PMenuParam)lParam;
	int result;

	if (!isGenMenuInited) return -1;
	if (pmp==NULL){return(-1);}
	lockmo();
	MenuObjects=(PIntMenuObject)mir_realloc(MenuObjects,sizeof(TIntMenuObject)*(MenuObjectsCount+1));
	memset(&(MenuObjects[MenuObjectsCount]),0,sizeof(MenuObjects[MenuObjectsCount]));
	MenuObjects[MenuObjectsCount].id=NextObjectId++;
	MenuObjects[MenuObjectsCount].Name=mir_strdup(pmp->name);
	MenuObjects[MenuObjectsCount].CheckService=mir_strdup(pmp->CheckService);
	MenuObjects[MenuObjectsCount].ExecService=mir_strdup(pmp->ExecService);

	//if (IsWinVerXPPlus())		//need 32-bit icons on XP for alpha channels
	MenuObjects[MenuObjectsCount].hMenuIcons=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,15,100);
	//else	  //Win2k won't blend icons with imagelist_drawex when color-depth>16-bit. Don't know about WinME, but it certainly doesn't support alpha channels
	//  MenuObjects[MenuObjectsCount].hMenuIcons=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR16|ILC_MASK,15,100);

	result=MenuObjects[MenuObjectsCount].id;
	MenuObjectsCount++;
	unlockmo();
	//	TRACE("!!! Created New MenuObject \t");
	//	if (pmp->name) TRACE(pmp->name);
	//	TRACE("\r\n");

	return(result);
}
//wparam=MenuItemHandle
//lparam=0
int MO_RemoveMenuItem(WPARAM wParam,LPARAM lParam)
{

	int objidx,menuitemidx;//pos in array
	ListParam lp;
	int res;

	lockmo();
	if (GetAllIdx(wParam,&objidx,&menuitemidx)==0){unlockmo();return(0);}

	lp.MenuObjectHandle=MenuObjects[objidx].id;
	res=RecursiveRemoveChilds(menuitemidx,&lp);
	unlockmo();
	return(res);
}

//wparam=MenuObjectHandle
//lparam=PMO_MenuItem
//return MenuItemHandle
int MO_AddNewMenuItem(WPARAM wParam,LPARAM lParam)
{
	PMO_MenuItem pmi=(PMO_MenuItem)lParam;
	int objidx,menuobjecthandle;
	int miidx,result;
	int res;

	if (!isGenMenuInited) return -1;
	if (pmi==NULL){return(-1);}
	lockmo();
	menuobjecthandle=wParam;
	objidx=GetMenuObjbyId(menuobjecthandle);
	if (objidx==-1){unlockmo();return(-1);}

	if (pmi->cbSize!=sizeof(TMO_MenuItem)) {unlockmo();return -1;}

	if (!(pmi->flags&CMIF_ROOTPOPUP||pmi->flags&CMIF_CHILDPOPUP)){
		//old mode
		unlockmo();
		res=(int)MO_AddOldNewMenuItem(wParam,lParam);
		return(res);
	}

	MenuObjects[objidx].MenuItems=(PMO_IntMenuItem)mir_realloc(MenuObjects[objidx].MenuItems,sizeof(TMO_IntMenuItem)*(MenuObjects[objidx].MenuItemsCount+1));
	miidx=MenuObjects[objidx].MenuItemsCount;

	ZeroMemory(&MenuObjects[objidx].MenuItems[miidx],sizeof(MenuObjects[objidx].MenuItems[miidx]));
	MenuObjects[objidx].MenuItems[miidx].id=NextObjectMenuItemId++;
	MenuObjects[objidx].MenuItems[miidx].globalid=getGlobalId(menuobjecthandle,MenuObjects[objidx].MenuItems[miidx].id);
	MenuObjects[objidx].MenuItems[miidx].mi=*pmi;
	MenuObjects[objidx].MenuItems[miidx].mi.pszName = (TCHAR*)CallService( MS_LANGPACK_PCHARTOTCHAR,0,(LPARAM)pmi->pszName);
	MenuObjects[objidx].MenuItems[miidx].iconId=-1;
	MenuObjects[objidx].MenuItems[miidx].OverrideShow=TRUE;
	MenuObjects[objidx].MenuItems[miidx].IconRegistred=FALSE;
	if (pmi->hIcon!=NULL) 
		MenuObjects[objidx].MenuItems[miidx].iconId=ImageList_AddIcon(MenuObjects[objidx].hMenuIcons,pmi->hIcon);

	result=getGlobalId(wParam,MenuObjects[objidx].MenuItems[miidx].id);
	MenuObjects[objidx].MenuItemsCount++;
	unlockmo();
	return(result);
}

//wparam=MenuObjectHandle
//lparam=PMO_MenuItem
int MO_AddOldNewMenuItem(WPARAM wParam,LPARAM lParam)
{
	PMO_MenuItem pmi=(PMO_MenuItem)lParam;
	int objidx;
	int oldroot,i;

	if (!isGenMenuInited) return -1;
	if (pmi==NULL){return(-1);}
	objidx=GetMenuObjbyId(wParam);
	if (objidx==-1){return(-1);}

	if (pmi->cbSize!=sizeof(TMO_MenuItem)) return 0;

	if ((pmi->flags&CMIF_ROOTPOPUP||pmi->flags&CMIF_CHILDPOPUP))	{return(0);}

	//is item with popup or not
	if (pmi->root==0)
	{
		//yes,this without popup
		pmi->root=-1;//first level
	}
	else
	{
		//no,search for needed root and create it if need
		TCHAR* tszRoot = ( TCHAR* )CallService( MS_LANGPACK_PCHARTOTCHAR, 0, (LPARAM)pmi->root );

		oldroot=-1;
		for(i=0;i<MenuObjects[objidx].MenuItemsCount;i++) {
			TMO_IntMenuItem *p = &MenuObjects[objidx].MenuItems[i];
			if ( p->mi.pszName == NULL )
				continue;
			if (( p->mi.flags & CMIF_ROOTPOPUP) && !_tcscmp( p->mi.pszName, tszRoot)) {
				oldroot = getGlobalId( MenuObjects[objidx].id, p->id );
				break;
		}	}
		mir_free( tszRoot );
		if (oldroot==-1)
		{
			//not found,creating root
			TMO_MenuItem tmi;
			ZeroMemory(&tmi,sizeof(tmi));
			tmi=*(PMO_MenuItem)lParam;
			tmi.flags|=CMIF_ROOTPOPUP;
			tmi.ownerdata=0;
			tmi.root=-1;
			//copy pszPopupName
			tmi.pszName=(TCHAR*)pmi->root;
			oldroot=MO_AddNewMenuItem(wParam,(LPARAM)&tmi);
		}
		pmi->root=oldroot;
		//popup will be created in next commands
	}
	pmi->flags|=CMIF_CHILDPOPUP;
	//add popup(root allready exists)
	return(MO_AddNewMenuItem(wParam,(LPARAM)pmi));
}

static int WhereToPlace(HMENU hMenu,PMO_MenuItem mi,MENUITEMINFO *mii,ListParam *param)
{
	int i=0;

	mii->fMask=MIIM_SUBMENU|MIIM_DATA;
	for ( i=GetMenuItemCount(hMenu)-1; i >= 0; i-- ) {
		GetMenuItemInfo(hMenu,i,TRUE,mii);
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
	int thisItemPosition,needSeparator,itemidx;
	MENUITEMINFO mii;
	PMO_IntMenuItem MenuItems=NULL;
	PMO_IntMenuItem pimi=NULL;

	int objid,menuitemid;//id
	int objidx,menuitemidx;//pos in array
	needSeparator=0;

	UnpackGlobalId(lpmii->dwItemData,&objid,&menuitemid);
	if ((objid==-1)||(menuitemid==-1)){return;}
	objidx=GetMenuObjbyId(objid);
	menuitemidx=GetMenuItembyId(objidx,menuitemid);
	if ((objidx==-1)||(menuitemidx==-1)){return;}

	MenuItems=MenuObjects[objidx].MenuItems;
	itemidx=menuitemidx;
	//timi=
	thisItemPosition=(MenuItems)[itemidx].mi.position;

	ZeroMemory(&mii,sizeof(mii));
	mii.cbSize=MENUITEMINFO_V4_SIZE;
	//check for separator before
	if (uItem) {
		mii.fMask=MIIM_SUBMENU|MIIM_DATA|MIIM_TYPE;
		GetMenuItemInfo(hMenu,uItem-1,TRUE,&mii);
		pimi=MO_GetIntMenuItem(mii.dwItemData);
		if (pimi!=NULL)
			if (mii.fType==MFT_SEPARATOR) needSeparator=0;
			else needSeparator=((pimi->mi.position)/SEPARATORPOSITIONINTERVAL)!=thisItemPosition/SEPARATORPOSITIONINTERVAL;
			if (needSeparator) {
				//but might be supposed to be after the next one instead
				mii.fType=0;
				if (uItem<GetMenuItemCount(hMenu)) {
					mii.fMask=MIIM_SUBMENU|MIIM_DATA|MIIM_TYPE;
					GetMenuItemInfo(hMenu,uItem,TRUE,&mii);
				}
				if (mii.fType!=MFT_SEPARATOR) {
					mii.fMask=MIIM_TYPE;
					mii.fType=MFT_SEPARATOR;
					InsertMenuItem(hMenu,uItem,TRUE,&mii);
				}
				uItem++;
			}
	}
	//check for separator after
	if (uItem<GetMenuItemCount(hMenu)) {
		mii.fMask=MIIM_SUBMENU|MIIM_DATA|MIIM_TYPE;
		mii.cch=0;
		GetMenuItemInfo(hMenu,uItem,TRUE,&mii);
		pimi=MO_GetIntMenuItem(mii.dwItemData);
		if (pimi!=NULL)
			if (mii.fType==MFT_SEPARATOR) needSeparator=0;
			else needSeparator=((pimi->mi.position)/SEPARATORPOSITIONINTERVAL)!=thisItemPosition/SEPARATORPOSITIONINTERVAL;
			if (needSeparator) {
				mii.fMask=MIIM_TYPE;
				mii.fType=MFT_SEPARATOR;
				InsertMenuItem(hMenu,uItem,TRUE,&mii);
			}
	}
	if (uItem==GetMenuItemCount(hMenu)-1) {
		TCHAR str[32];
		mii.fMask=MIIM_SUBMENU|MIIM_DATA|MIIM_TYPE;
		mii.dwTypeData=str;
		mii.cch=SIZEOF(str);
		GetMenuItemInfo(hMenu,uItem,TRUE,&mii);
		/*if (param->cntFlag==MENU_CUSTOMITEMMAIN) {
		if (mii.fType==MFT_STRING && !strcmp(mii.dwTypeData,Translate("E&xit"))) {
		//make sure we keep the separator before the exit menu item
		mii.fMask=MIIM_TYPE;
		mii.fType=MFT_SEPARATOR;
		InsertMenuItem(hMenu,uItem,TRUE,&mii);
		}

		}
		*/
	}
	InsertMenuItem(hMenu,uItem,TRUE,lpmii);
}
//wparam started hMenu
//lparam ListParam*
//result hMenu
int MO_BuildMenu(WPARAM wParam,LPARAM lParam)
{
	int res;
	int tick;
	int pimoidx;
	ListParam * lp;

	if (!isGenMenuInited) return -1;
	lockmo();

	tick=GetTickCount();
	lp=(ListParam *)lParam;
	pimoidx=GetMenuObjbyId(lp->MenuObjectHandle);
	if (pimoidx==-1){return(0);}

	res=(int)BuildRecursiveMenu((HMENU)wParam,(ListParam *)lParam);

	tick=GetTickCount()-tick;
	{
		char buf[256];
		wsprintfA(buf,"build %s, %d ms\r\n",MenuObjects[pimoidx].Name,tick);
	}
	unlockmo();

	return(res);
}

#ifdef _DEBUG
#define PUTPOSITIONSONMENU
#endif

/**************************************/
/*
{

char *menuItemName = mi->pszName;

char DBString[256];
DBVARIANT dbv;

// check if it visible
wsprintf(DBString, "%s_visible", menuItemName);
DBWriteContactSettingByte(NULL, "MainMenuItems", DBString,DBGetContactSettingByte(NULL, "MainMenuItems", DBString, 1));
if (!DBGetContactSettingByte(NULL, "MainMenuItems", DBString, 1))
return 0;  // find out what value to return if not getting added


// mi.pszName
wsprintf(DBString, "%s_name", menuItemName);
if (!DBGetContactSetting(NULL, "MainMenuItems", DBString, &dbv))
mi->pszName = strdup(dbv.pszVal);

DBFreeVariant(&dbv);

}
*/
/**************************************/

void GetMenuItemName( PMO_IntMenuItem pMenuItem, char* pszDest, size_t cbDestSize )
{
	if ( pMenuItem->UniqName)
		mir_snprintf( pszDest, cbDestSize, "{%s}", pMenuItem->UniqName );
	else {
		#if defined( _UNICODE )
			char* name = u2a(pMenuItem->mi.pszName);
			mir_snprintf( pszDest, cbDestSize, "{%s}", name );
			free(name);
		#else
			mir_snprintf( pszDest, cbDestSize, "{%s}", pMenuItem->mi.pszName );
		#endif
}	}

HMENU BuildRecursiveMenu(HMENU hMenu,ListParam *param)
{
	int i,j;
	MENUITEMINFO mii;
	PMO_MenuItem mi;
	HMENU hSubMenu;
	ListParam localparam;
	TCheckProcParam CheckParam;
	//PIntMenuObject pimo=NULL;
	int pimoidx;

	PMO_IntMenuItem MenuItems=NULL;
	int rootlevel;
	//	int cntFlag;
	int MenuItemsCount;
	char *checkproc=NULL;
	char *onAddproc=NULL;
	char MenuNameItems[256];

	if (param==NULL) return(NULL);
	pimoidx=GetMenuObjbyId(param->MenuObjectHandle);
	if (pimoidx==-1){return(NULL);}
	//pimo=&MenuObjects[pimoidx];

	MenuItems=MenuObjects[pimoidx].MenuItems;
	MenuItemsCount=MenuObjects[pimoidx].MenuItemsCount;
	rootlevel=param->rootlevel;
	//cntFlag=param->cntFlag;
	checkproc=MenuObjects[pimoidx].CheckService;
	onAddproc=MenuObjects[pimoidx].onAddService;

	localparam=*param;
	wsprintfA(MenuNameItems, "%s_Items", MenuObjects[pimoidx].Name);
	/*
	while((rootlevel==-1)&&(GetMenuItemCount(hMenu)>(param->cntFlag==MENU_CUSTOMITEMMAIN?1:0)) )
	DeleteMenu(hMenu,0,MF_BYPOSITION);
	*/
	while((rootlevel==-1)&&GetMenuItemCount(hMenu)>0) DeleteMenu(hMenu,0,MF_BYPOSITION);

	for(j=0;j<MenuItemsCount;j++) {
		mi=&(MenuItems[j].mi);
		if (mi->cbSize!=sizeof(TMO_MenuItem)) continue;

		//
		//		RegisterOneIcon(pimoidx,j);

		if (checkproc!=NULL)	{
			CheckParam.lParam=param->lParam;
			CheckParam.wParam=param->wParam;
			CheckParam.MenuItemOwnerData=mi->ownerdata;
			CheckParam.MenuItemHandle=getGlobalId(param->MenuObjectHandle,MenuItems[j].id);
			if (CallService(checkproc,(WPARAM)&CheckParam,(LPARAM)0)==FALSE)	continue;
		}

		/**************************************/
		if (rootlevel==-1&&mi->root==-1&&MenuObjects[pimoidx].bUseUserDefinedItems) {
			char menuItemName[256];
			char DBString[256];
			DBVARIANT dbv;
			memset(&dbv,0,sizeof(dbv));

			GetMenuItemName( &MenuItems[j], menuItemName, sizeof(menuItemName));

			// check if it visible
			wsprintfA(DBString, "%s_visible", menuItemName);
			if (DBGetContactSettingByte(NULL, MenuNameItems, DBString, -1)==-1)
				DBWriteContactSettingByte(NULL,MenuNameItems, DBString,1);

			MenuItems[j].OverrideShow=TRUE;
			if (!DBGetContactSettingByte(NULL, MenuNameItems, DBString, 1)) {
				MenuItems[j].OverrideShow=FALSE;
				continue;  // find out what value to return if not getting added
			}

			// mi.pszName
			wsprintfA(DBString, "%s_name", menuItemName);
			if (!DBGetContactSettingTString(NULL, MenuNameItems, DBString, &dbv)) {
				if (_tcslen(dbv.ptszVal)>0) {
					if (MenuItems[j].CustomName) mir_free(MenuItems[j].CustomName);
					MenuItems[j].CustomName=mir_tstrdup(dbv.ptszVal);
				}
				if (dbv.ptszVal)	mir_free(dbv.ptszVal);
			}

			wsprintfA(DBString, "%s_pos", menuItemName);
			if (DBGetContactSettingDword(NULL, MenuNameItems, DBString, -1)==-1)
				DBWriteContactSettingDword(NULL,MenuNameItems, DBString,mi->position);

			mi->position = DBGetContactSettingDword(NULL, MenuNameItems, DBString, mi->position);
		}

		/**************************************/

		ZeroMemory(&mii,sizeof(mii));
		i=0;
		//hMenu=hMainMenu;
		mii.cbSize=MENUITEMINFO_V4_SIZE;
		mii.fMask=MIIM_SUBMENU|MIIM_TYPE|MIIM_DATA;
		mii.dwItemData=getGlobalId(param->MenuObjectHandle,MenuItems[j].id);
		hSubMenu=NULL;

		if (rootlevel!=(int)MenuItems[j].mi.root)
			continue;

		if ((MenuItems[j].mi.flags&CMIF_CHILDPOPUP)||(MenuItems[j].mi.flags&CMIF_ROOTPOPUP))	{
			if (rootlevel==(int)MenuItems[j].mi.root) {
				//our level
				if (MenuItems[j].mi.flags&CMIF_ROOTPOPUP) {
					i=WhereToPlace(hMenu,mi,&mii,&localparam);

					if (!IsWinVer98Plus()) {
						mii.cbSize=MENUITEMINFO_V4_SIZE;
						mii.fMask=MIIM_DATA|MIIM_TYPE|MIIM_ID|MIIM_SUBMENU;
					}
					else {
						mii.cbSize=sizeof(mii);
						mii.fMask=MIIM_DATA|MIIM_ID|MIIM_STRING|MIIM_SUBMENU;
						if (MenuItems[j].iconId!=-1) mii.fMask|=MIIM_BITMAP;
					}

					mii.fType=MFT_STRING;
					//mii.dwItemData=getGlobalId(param->MenuObjectHandle,MenuItems[j].id);
					mii.dwItemData=MenuItems[j].globalid;

					hSubMenu=CreatePopupMenu();
					mii.hSubMenu=hSubMenu;
					mii.hbmpItem=HBMMENU_CALLBACK;
					mii.dwTypeData=(MenuItems[j].CustomName)?(MenuItems[j].CustomName):(mi->pszName);
#ifdef PUTPOSITIONSONMENU
					if (GetKeyState(VK_CONTROL)&0x8000) {
						TCHAR str[256];
						mir_sntprintf(str,SIZEOF(str),_T("%s (%d,id %x)"),mi->pszName,mi->position,mii.dwItemData);
						mii.dwTypeData=str;
					}
#endif
					InsertMenuItemWithSeparators(hMenu,i,TRUE,&mii,&localparam);
					localparam.rootlevel=getGlobalId(param->MenuObjectHandle,MenuItems[j].id);//MenuItems[j].id|cntFlag;
					BuildRecursiveMenu(hSubMenu,&localparam);

					continue;
				}

				i=WhereToPlace(hMenu,mi,&mii,&localparam);

				if (!IsWinVer98Plus()) {
					mii.cbSize=MENUITEMINFO_V4_SIZE;
					mii.fMask=MIIM_DATA|MIIM_TYPE|MIIM_ID|MIIM_STATE;
				}
				else {
					mii.cbSize=sizeof(mii);
					mii.fMask=MIIM_DATA|MIIM_ID|MIIM_STRING|MIIM_STATE;
					if (MenuItems[j].iconId!=-1) mii.fMask|=MIIM_BITMAP;
				}
				mii.fState=((MenuItems[j].mi.flags&CMIF_GRAYED)?MFS_GRAYED:MFS_ENABLED);
				mii.fState|=((MenuItems[j].mi.flags&CMIF_CHECKED)?MFS_CHECKED:MFS_UNCHECKED);
				mii.fType=MFT_STRING;
				mii.wID=MenuItems[j].id;
				//mii.dwItemData=getGlobalId(param->MenuObjectHandle,MenuItems[j].id);//MenuItems[j].id|cntFlag;
				mii.dwItemData=MenuItems[j].globalid;

				mii.hbmpItem=HBMMENU_CALLBACK;
				mii.dwTypeData=(MenuItems[j].CustomName)?(MenuItems[j].CustomName):(mi->pszName);
#ifdef PUTPOSITIONSONMENU
				if (GetKeyState(VK_CONTROL)&0x8000) {
					TCHAR str[256];
					mir_sntprintf( str, SIZEOF(str), _T("%s (%d,id %x)"),mi->pszName,mi->position,mii.dwItemData);
					mii.dwTypeData=str;
				}
#endif
				if (onAddproc!=NULL)
					if (CallService(onAddproc,(WPARAM)&mii,(LPARAM)MenuItems[j].globalid)==FALSE)	continue;

				InsertMenuItemWithSeparators(hMenu,i,TRUE,&mii,&localparam);
				continue;
			}
		}
		else {
			//old items - not needed
			if (rootlevel==-1) {
			}
		}
	}
	return hMenu;
}

static int RemoveFromList(int pos,void **lpList,int *ListElemCount,int ElemSize)
{
	//memcpy(&((*lpMenuItems)[pos]),&((*lpMenuItems)[pos+1]),sizeof(CListIntMenuItem)*(*MenuItemCount-pos-1));
	void *p1,*p2;
	p1=(void *)( (int)(*lpList)+pos*ElemSize);
	p2=(void *)( (int)(*lpList)+(pos+1)*ElemSize);
	memcpy(p1,p2,ElemSize*(*ListElemCount-pos-1));
	(*ListElemCount)--;
	(*lpList)=mir_realloc((*lpList),ElemSize*(*ListElemCount));
	return 0;
}

int RecursiveRemoveChilds(int pos,ListParam *param)
{
	ListParam localparam;
	int rootid;
	int i=0;
	int menuitemid;//id
	int objidx;//pos in array
	int *MenuItemsCount;
	PMO_IntMenuItem *MenuItems=NULL;

	objidx=GetMenuObjbyId(param->MenuObjectHandle);
	if ((objidx==-1)){return(-1);}

	localparam=*param;

	//rootid=(*param->MenuItems)[pos].id|param->cntFlag;
	rootid=getGlobalId(MenuObjects[objidx].id,MenuObjects[objidx].MenuItems[pos].id);
	menuitemid=MenuObjects[objidx].MenuItems[pos].id;

	MenuItems=&MenuObjects[objidx].MenuItems;
	MenuItemsCount=&MenuObjects[objidx].MenuItemsCount;

	if ((*MenuItems)[pos].mi.flags&CMIF_ROOTPOPUP) {
		while(i<*MenuItemsCount) {
			if ((*MenuItems)[i].mi.root!=rootid||(*MenuItems)[i].mi.root==-1) {
				i++;
				continue;
			}
			if ((*MenuItems)[i].mi.flags&CMIF_ROOTPOPUP) {
				RecursiveRemoveChilds(i,&localparam);
				i=0;
				continue;
			}
			if ((*MenuItems)[i].mi.flags&CMIF_CHILDPOPUP) {
				FreeAndNil(&((*MenuItems)[i].mi.pszName));
				FreeAndNil(&((*MenuItems)[i].CustomName));
				FreeAndNil(&((*MenuItems)[i].UniqName));
				if (MenuObjects[objidx].FreeService)
					CallService(MenuObjects[objidx].FreeService,(WPARAM)(*MenuItems)[i].globalid,(LPARAM)(*MenuItems)[i].mi.ownerdata);

				RemoveFromList(i,(void **)MenuItems,MenuItemsCount,sizeof(TMO_IntMenuItem));
				i=0;
				continue;
	}	}	}

	i = GetMenuItembyId( objidx, menuitemid );
	if ( i >= 0 && i < *MenuItemsCount ) {
		FreeAndNil(&((*MenuItems)[i].mi.pszName));
		FreeAndNil(&((*MenuItems)[i].CustomName));
		FreeAndNil(&((*MenuItems)[i].UniqName));

		if (MenuObjects[objidx].FreeService)
			CallService(MenuObjects[objidx].FreeService,(WPARAM)(*MenuItems)[i].globalid,(LPARAM)(*MenuItems)[i].mi.ownerdata);

		RemoveFromList(i,(void **)MenuItems,MenuItemsCount,sizeof(TMO_IntMenuItem));
	}
	return 0;
}

//#define PostRegisterTimerID 12001
int posttimerid;
VOID CALLBACK PostRegisterIcons( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	KillTimer(0,posttimerid);
}

int OnModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	//RegisterAllIconsinIconLib();
	//CallService(MS_CLUI_GETHWND,0,0)
	posttimerid=SetTimer((HWND)NULL,0,5,(TIMERPROC)PostRegisterIcons);
	return 0;
}

int InitGenMenu()
{
	InitializeCriticalSection(&csMenuHook);
	CreateServiceFunction(MO_BUILDMENU,MO_BuildMenu);

	CreateServiceFunction(MO_PROCESSCOMMAND,MO_ProcessCommand);
	CreateServiceFunction(MO_CREATENEWMENUOBJECT,MO_CreateNewMenuObject);
	CreateServiceFunction(MO_REMOVEMENUITEM,MO_RemoveMenuItem);
	CreateServiceFunction(MO_ADDNEWMENUITEM,MO_AddNewMenuItem);
	CreateServiceFunction(MO_MENUITEMGETOWNERDATA,MO_MenuItemGetOwnerData);
	CreateServiceFunction(MO_MODIFYMENUITEM,MO_ModifyMenuItem);
	CreateServiceFunction(MO_GETMENUITEM,MO_GetMenuItem);
	CreateServiceFunction(MO_PROCESSCOMMANDBYMENUIDENT,MO_ProcessCommandByMenuIdent);
	CreateServiceFunction(MO_PROCESSHOTKEYS,MO_ProcessHotKeys);
	CreateServiceFunction(MO_REMOVEMENUOBJECT,MO_RemoveMenuObject);

	CreateServiceFunction(MO_DRAWMENUITEM,MO_DrawMenuItem);
	CreateServiceFunction(MO_MEASUREMENUITEM,MO_MeasureMenuItem);

	CreateServiceFunction(MO_SETOPTIONSMENUOBJECT,MO_SetOptionsMenuObject);
	CreateServiceFunction(MO_SETOPTIONSMENUITEM,MO_SetOptionsMenuItem);

	lockmo();
	isGenMenuInited=TRUE;
	unlockmo();

	HookEvent(ME_SYSTEM_MODULESLOADED,OnModulesLoaded);
	return(0);
}

int UnitGenMenu()
{
	lockmo();
	MO_RemoveAllObjects();
	isGenMenuInited=FALSE;

	DestroyServiceFunction(MO_BUILDMENU);

	DestroyServiceFunction(MO_PROCESSCOMMAND);
	DestroyServiceFunction(MO_CREATENEWMENUOBJECT);
	DestroyServiceFunction(MO_REMOVEMENUITEM);
	DestroyServiceFunction(MO_ADDNEWMENUITEM);
	DestroyServiceFunction(MO_MENUITEMGETOWNERDATA);
	DestroyServiceFunction(MO_MODIFYMENUITEM);
	DestroyServiceFunction(MO_GETMENUITEM);
	DestroyServiceFunction(MO_PROCESSCOMMANDBYMENUIDENT);
	DestroyServiceFunction(MO_PROCESSHOTKEYS);
	DestroyServiceFunction(MO_REMOVEMENUOBJECT);

	DestroyServiceFunction(MO_DRAWMENUITEM);
	DestroyServiceFunction(MO_MEASUREMENUITEM);

	DestroyServiceFunction(MO_SETOPTIONSMENUOBJECT);
	DestroyServiceFunction(MO_SETOPTIONSMENUITEM);
	DestroyHookableEvent(ME_OPT_INITIALISE);
	unlockmo();

	DeleteCriticalSection(&csMenuHook);
	return(0);
}
