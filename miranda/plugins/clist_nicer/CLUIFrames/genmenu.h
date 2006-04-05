#ifndef GENMENU_H
#define GENMENU_H
//general menu object module
#include "m_genmenu.h"

typedef struct
{
	int id;
	int globalid;

	int iconId;
	TMO_MenuItem mi;
	boolean OverrideShow;
	char *UniqName;
	TCHAR *CustomName;
	boolean IconRegistred;
}
	TMO_IntMenuItem,*PMO_IntMenuItem;

typedef struct
{
	char *Name;//for debug purposes
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
	HANDLE hMenuIcons;
	BOOL bUseUserDefinedItems;
}
	TIntMenuObject,*PIntMenuObject;


#define SEPARATORPOSITIONINTERVAL	100000

//internal usage
HMENU BuildRecursiveMenu(HMENU hMenu,ListParam *param);
int RecursiveRemoveChilds(int pos,ListParam *param);
PMO_IntMenuItem MO_GetIntMenuItem(int globid);
int MO_AddOldNewMenuItem(WPARAM wParam,LPARAM lParam);
void UnpackGlobalId(int id,int *MenuObjectId,int *MenuItemId);
void GetMenuItemName( PMO_IntMenuItem pMenuItem, char* pszDest, size_t cbDestSize );

//for old processcommand
int getGlobalId(const int MenuObjectId,const int MenuItemId);

//general stuff
int InitGenMenu();
int UnitGenMenu();

int GenMenuOptInit(WPARAM wParam,LPARAM lParam);
int GetMenuObjbyId(const int id);
int GetMenuItembyId(const int objpos,const int id);
int MO_GetMenuItem(WPARAM wParam,LPARAM lParam);
void FreeAndNil(void **p);
static int RemoveFromList(int pos,void **lpList,int *ListElemCount,int ElemSize);
int MO_ProcessCommand(WPARAM wParam,LPARAM lParam);
static int RemoveFromList(int pos,void **lpList,int *ListElemCount,int ElemSize);
#endif