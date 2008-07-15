#ifndef GENMENU_H
#define GENMENU_H
//general menu object module
#include "m_genmenu.h"

typedef struct
{
	char* Name;
	int id;

	//ExecService
	//LPARAM lParam;//owner data
	//WPARAM wParam;//allways lparam from winproc
	char *ExecService;

	//CheckService called when building menu
	//return false to skip item.
	//LPARAM lParam;//0
	//WPARAM wParam;//CheckParam
	char *CheckService;//analog to check_proc

	//LPARAM lParam;//ownerdata
	//WPARAM wParam;//menuitemhandle
	char *FreeService;//callback service used to free ownerdata for menuitems

	//LPARAM lParam;//MENUITEMINFO filled with all needed data
	//WPARAM wParam;//menuitemhandle
	char *onAddService;//called just before add MENUITEMINFO to hMenu

	PMO_IntMenuItem MenuItems;
	int MenuItemsCount;
	HIMAGELIST hMenuIcons;
	BOOL bUseUserDefinedItems;
}
	TIntMenuObject,*PIntMenuObject;


#define SEPARATORPOSITIONINTERVAL	100000

//internal usage
HMENU BuildRecursiveMenu(HMENU hMenu,ListParam *param);
int RecursiveRemoveChilds(int pos,ListParam *param);
void UnpackGlobalId(int id,int *MenuObjectId,int *MenuItemId);
void GetMenuItemName( PMO_IntMenuItem pMenuItem, char* pszDest, size_t cbDestSize );

PMO_IntMenuItem MO_GetIntMenuItem(int globid);

int MO_AddNewMenuItem( int menuobjecthandle, PMO_MenuItem pmi );
int MO_AddOldNewMenuItem( int menuobjecthandle, PMO_MenuItem pmi );
int MO_DrawMenuItem( LPDRAWITEMSTRUCT dis );
int MO_MeasureMenuItem( LPMEASUREITEMSTRUCT mis );
int MO_ModifyMenuItem( int menuHandle, PMO_MenuItem pmiparam );
int MO_ProcessCommand( WPARAM wParam, LPARAM lParam );
int MO_ProcessHotKeys( int menuHandle, int vKey );
int MO_SetOptionsMenuItem( int menuobjecthandle, int setting, int value );
int MO_SetOptionsMenuObject( int menuobjecthandle, int setting, int value );

//for old processcommand
int getGlobalId(const int MenuObjectId,const int MenuItemId);

//general stuff
int InitGenMenu();
int UnitGenMenu();

TMO_IntMenuItem * GetMenuItemByGlobalID(int globalMenuID);
BOOL	FindMenuHanleByGlobalID(HMENU hMenu, int globalID, struct _MenuItemHandles * dat);	//GenMenu.c

int GenMenuOptInit(WPARAM wParam, LPARAM lParam);
int GetMenuObjbyId(const int id);
int GetMenuItembyId(const int objpos,const int id);
int MO_GetMenuItem(WPARAM wParam,LPARAM lParam);
void FreeAndNil(void **p);
static int RemoveFromList(int pos,void **lpList,int *ListElemCount,int ElemSize);
static int RemoveFromList(int pos,void **lpList,int *ListElemCount,int ElemSize);
#endif
